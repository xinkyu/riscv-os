#include "defs.h"
#include "param.h"
#include "buf.h"
#include "fs.h"
#include "virtio.h"

#define NUM 8

struct virtio_blk_req {
    uint32 type;
    uint32 reserved;
    uint64 sector;
};

struct disk {
    struct spinlock lock;
    struct virtq_desc *desc;
    struct virtq_avail *avail;
    struct virtq_used *used;
    char free[NUM];
    uint16 used_idx;

    struct {
        struct buf *b;
        char status;
        struct virtio_blk_req cmd;
    } info[NUM];
} disk;

static uint64 disk_reads;
static uint64 disk_writes;

static inline volatile uint32 *mmio_reg(int off) {
    return (volatile uint32 *)((uint64)VIRTIO0 + off);
}

static inline uint32 r32(int off) {
    return *mmio_reg(off);
}

static inline void w32(int off, uint32 val) {
    *mmio_reg(off) = val;
}

static int alloc_desc(void) {
    for (int i = 0; i < NUM; i++) {
        if (disk.free[i]) {
            disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

static void free_desc(int i) {
    if (i >= NUM) {
        panic("free_desc");
    }
    disk.desc[i].addr = 0;
    disk.desc[i].len = 0;
    disk.desc[i].flags = 0;
    disk.desc[i].next = 0;
    disk.free[i] = 1;
}

static void free_chain(int i) {
    while (1) {
        int flag = disk.desc[i].flags;
        int next = disk.desc[i].next;
        free_desc(i);
        if (!(flag & 1)) {
            break;
        }
        i = next;
    }
}

void virtio_disk_init(void) {
    spinlock_init(&disk.lock, "virtio_disk");

    uint32 magic = r32(VIRTIO_MMIO_MAGIC_VALUE);
    uint32 version = r32(VIRTIO_MMIO_VERSION);
    uint32 device_id = r32(VIRTIO_MMIO_DEVICE_ID);
    uint32 vendor_id = r32(VIRTIO_MMIO_VENDOR_ID);

    if (magic != 0x74726976 ||
        version != 2 ||
        device_id != 2 ||
        vendor_id != 0x554d4551) {
        printf("virtio: magic=0x%x version=0x%x device=0x%x vendor=0x%x\n", magic, version, device_id, vendor_id);
        panic("virtio_disk_init: cannot find virtio disk");
    }

    w32(VIRTIO_MMIO_STATUS, 0);
    w32(VIRTIO_MMIO_STATUS, 1);
    w32(VIRTIO_MMIO_STATUS, 1 | 2);
    w32(VIRTIO_MMIO_STATUS, 1 | 2 | 4);

    uint32 features = r32(VIRTIO_MMIO_DEVICE_FEATURES);
    features &= ~(1 << 28);
    w32(VIRTIO_MMIO_DRIVER_FEATURES, features);

    w32(VIRTIO_MMIO_STATUS, 1 | 2 | 4 | 8);

    w32(VIRTIO_MMIO_QUEUE_SEL, 0);
    uint32 max = r32(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0) {
        panic("virtio_disk_init: no queue 0");
    }
    if (max < NUM) {
        panic("virtio_disk_init: queue too short");
    }
    w32(VIRTIO_MMIO_QUEUE_NUM, NUM);

    disk.desc = (struct virtq_desc*)kalloc();
    disk.avail = (struct virtq_avail*)kalloc();
    disk.used = (struct virtq_used*)kalloc();
    if (!disk.desc || !disk.avail || !disk.used) {
        panic("virtio_disk_init: kalloc");
    }
    memset(disk.desc, 0, PGSIZE);
    memset(disk.avail, 0, PGSIZE);
    memset(disk.used, 0, PGSIZE);

    w32(VIRTIO_MMIO_QUEUE_DESC_LOW, (uint64)disk.desc);
    w32(VIRTIO_MMIO_QUEUE_DESC_HIGH, (uint64)disk.desc >> 32);
    w32(VIRTIO_MMIO_DRIVER_DESC_LOW, (uint64)disk.avail);
    w32(VIRTIO_MMIO_DRIVER_DESC_HIGH, (uint64)disk.avail >> 32);
    w32(VIRTIO_MMIO_DEVICE_DESC_LOW, (uint64)disk.used);
    w32(VIRTIO_MMIO_DEVICE_DESC_HIGH, (uint64)disk.used >> 32);

    w32(VIRTIO_MMIO_QUEUE_READY, 1);

    for (int i = 0; i < NUM; i++) {
        disk.free[i] = 1;
    }
    disk.used_idx = 0;
}

void virtio_disk_rw(struct buf *b, int write) {
    int idx[3];
    acquire(&disk.lock);

    for (int i = 0; i < 3; i++) {
        while ((idx[i] = alloc_desc()) < 0) {
            // busy wait until desc free
        }
    }

    struct virtio_blk_req *cmd = &disk.info[idx[0]].cmd;
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = write ? 1 : 0;
    cmd->sector = (uint64)b->blockno * (BSIZE / 512);

    disk.desc[idx[0]].addr = (uint64)cmd;
    disk.desc[idx[0]].len = sizeof(*cmd);
    disk.desc[idx[0]].flags = 1; // NEXT
    disk.desc[idx[0]].next = idx[1];

    disk.desc[idx[1]].addr = (uint64)b->data;
    disk.desc[idx[1]].len = BSIZE;
    if (write)
        disk.desc[idx[1]].flags = 0;
    else
        disk.desc[idx[1]].flags = 2; // WRITE
    disk.desc[idx[1]].flags |= 1;
    disk.desc[idx[1]].next = idx[2];

    disk.info[idx[0]].status = 0xff;
    disk.desc[idx[2]].addr = (uint64)&disk.info[idx[0]].status;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = 2; // WRITE
    disk.desc[idx[2]].next = 0;

    b->disk = 1;
    disk.info[idx[0]].b = b;

    disk.avail->ring[disk.avail->idx % NUM] = idx[0];
    __sync_synchronize();
    disk.avail->idx++;

    __sync_synchronize();
    w32(VIRTIO_MMIO_QUEUE_NOTIFY, 0);

    if (write)
        disk_writes++;
    else
        disk_reads++;

    while (disk.info[idx[0]].status == 0xff) {
        while (disk.used_idx != disk.used->idx) {
            int id = disk.used->ring[disk.used_idx % NUM].id;
            disk.used_idx++;
            disk.info[id].status = 0;
            if (disk.info[id].b) {
                disk.info[id].b->disk = 0;
            }
        }
    }

    free_chain(idx[0]);

    release(&disk.lock);
}

void virtio_disk_intr(void) {
    acquire(&disk.lock);
    while (disk.used_idx != disk.used->idx) {
        int id = disk.used->ring[disk.used_idx % NUM].id;
        disk.used_idx++;
        disk.info[id].status = 0;
        struct buf *b = disk.info[id].b;
        b->disk = 0;
        wakeup(b);
    }
    release(&disk.lock);
}

uint64 get_disk_read_count(void) {
    return disk_reads;
}

uint64 get_disk_write_count(void) {
    return disk_writes;
}
