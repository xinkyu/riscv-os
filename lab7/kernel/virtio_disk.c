#include "defs.h"
#include "param.h"
#include "buf.h"
#include "fs.h"
#include "virtio.h"

//实现了对 virtio 块设备的读写操作。
//将内核的读写请求（request）放入 virtio 的描述符环（中，通知模拟硬件进行数据传输，并处理磁盘中断。
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

    printf("virtio: magic=0x%x version=0x%x device=0x%x vendor=0x%x\n", magic, version, device_id, vendor_id);

    if (magic != 0x74726976 || version != 2 || device_id != 2 || vendor_id != 0x554d4551) {
        panic("virtio_disk_init: cannot find virtio disk");
    }

    w32(VIRTIO_MMIO_STATUS, 0);
    w32(VIRTIO_MMIO_STATUS, r32(VIRTIO_MMIO_STATUS) | 1);
    w32(VIRTIO_MMIO_STATUS, r32(VIRTIO_MMIO_STATUS) | 2);

    uint32 features = r32(VIRTIO_MMIO_DEVICE_FEATURES);
    features &= ~(1 << 28);
    features &= ~(1 << 24);
    w32(VIRTIO_MMIO_DRIVER_FEATURES, features);

    w32(VIRTIO_MMIO_STATUS, r32(VIRTIO_MMIO_STATUS) | 4);
    if (!(r32(VIRTIO_MMIO_STATUS) & 4)) {
        panic("virtio_disk_init: features not accepted");
    }

    w32(VIRTIO_MMIO_STATUS, r32(VIRTIO_MMIO_STATUS) | 8);

    w32(VIRTIO_MMIO_QUEUE_SEL, 0);
    if (r32(VIRTIO_MMIO_QUEUE_READY)) {
        panic("virtio_disk_init: queue should not be ready");
    }

    uint32 max = r32(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0) panic("virtio_disk_init: no queue 0");
    if (max < NUM) panic("virtio_disk_init: queue too short");
    
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
            // 对于 Lab，这里简单的忙等待是可以的
            // release(&disk.lock); acquire(&disk.lock);
        }
    }

    struct virtio_blk_req *cmd = &disk.info[idx[0]].cmd;
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = write ? 1 : 0;
    cmd->reserved = 0;
    cmd->sector = (uint64)b->blockno * (BSIZE / 512);

    disk.desc[idx[0]].addr = (uint64)cmd;
    disk.desc[idx[0]].len = sizeof(*cmd);
    disk.desc[idx[0]].flags = 1; 
    disk.desc[idx[0]].next = idx[1];

    disk.desc[idx[1]].addr = (uint64)b->data;
    disk.desc[idx[1]].len = BSIZE;
    if (write)
        disk.desc[idx[1]].flags = 0;
    else
        disk.desc[idx[1]].flags = 2;
    disk.desc[idx[1]].flags |= 1;
    disk.desc[idx[1]].next = idx[2];

    disk.info[idx[0]].status = 0xff;
    disk.desc[idx[2]].addr = (uint64)&disk.info[idx[0]].status;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = 2;
    disk.desc[idx[2]].next = 0;

    // 防御检查
    if (disk.desc[idx[0]].len == 0 || disk.desc[idx[1]].len == 0 || disk.desc[idx[2]].len == 0) {
        panic("virtio: zero length descriptor");
    }

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

    // 使用轮询代替 sleep，以支持启动阶段 
    while (disk.info[idx[0]].status == 0xff) {
        // 释放锁，允许中断处理程序（如果有的话）运行
        release(&disk.lock);
        
        // 在启动阶段，中断可能还没完全接管，或者我们没有进程上下文来 sleep
        // 主动轮询设备状态是一个简单的解决方案
        if (r32(VIRTIO_MMIO_INTERRUPT_STATUS) & 1) {
            w32(VIRTIO_MMIO_INTERRUPT_ACK, r32(VIRTIO_MMIO_INTERRUPT_STATUS));
            virtio_disk_intr();
        }
        
        // 重新获取锁，检查状态
        acquire(&disk.lock);
    }

    disk.info[idx[0]].b = 0;
    free_chain(idx[0]);

    release(&disk.lock);
}

void virtio_disk_intr(void) {
    acquire(&disk.lock);

    __sync_synchronize();

    while (disk.used_idx != disk.used->idx) {
        __sync_synchronize();
        int id = disk.used->ring[disk.used_idx % NUM].id;
        disk.used_idx++;
        __sync_synchronize();

        if (id >= NUM) {
            // printf("virtio_disk_intr: bad id %d\n", id);
            continue; 
        }

        struct buf *b = disk.info[id].b;
        
        // 依然保留这个检查，以防万一
        if (b == 0) {
            // printf("virtio_disk_intr: b is 0\n");
            continue; 
        }

        b->disk = 0;
        
        // wakeup(b) 是安全的，即使在启动阶段调用它，只要它检查了 chan 是否为空
        // 但在我们的轮询模式下，wakeup 不是必须的，因为 rw 函数自己在轮询 status
        // 不过为了兼容 sleep 模式，还是保留它
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