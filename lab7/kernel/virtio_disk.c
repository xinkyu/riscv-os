// kernel/virtio_disk.c
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h" // 需要确保有 VIRTIO0 定义，或者直接用宏
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "virtio.h"

// the address of virtio mmio register r.
#define R(r) ((volatile uint32 *)(VIRTIO0 + (r)))

static struct disk {
  // 内存页大小为 4096
  char pages[2*PGSIZE];
  struct VRingDesc *desc;
  uint16 *avail;
  struct UsedArea *used;
  char free[8];  // free[i] is true if desc[i] is free
  uint16 used_idx; // we've looked this far in used[2..n].
  struct buf *info[8]; // track open buffers
  uint16 ops[8]; // commands
  struct spinlock vdisk_lock;
} disk;

struct VRingDesc {
  uint64 addr;
  uint32 len;
  uint16 flags;
  uint16 next;
};
#define VRING_DESC_F_NEXT  1
#define VRING_DESC_F_WRITE 2

struct UsedArea {
  uint16 flags;
  uint16 id;
  struct {
    uint32 id;
    uint32 len;
  } elems[8];
};

void virtio_disk_init(void) {
  uint32 status = 0;
  initlock(&disk.vdisk_lock, "virtio_disk");

  if(R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
     R(VIRTIO_MMIO_VERSION) != 1 ||
     R(VIRTIO_MMIO_DEVICE_ID) != 2 ||
     R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551){
    panic("could not find virtio disk");
  }
  
  status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  R(VIRTIO_MMIO_STATUS) = status;
  status |= VIRTIO_CONFIG_S_DRIVER;
  R(VIRTIO_MMIO_STATUS) = status;

  uint64 features = R(VIRTIO_MMIO_DEVICE_FEATURES);
  features &= ~(1 << VIRTIO_BLK_F_RO);
  features &= ~(1 << VIRTIO_BLK_F_SCSI);
  features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
  features &= ~(1 << VIRTIO_BLK_F_MQ);
  features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
  features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
  features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
  R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

  status |= VIRTIO_CONFIG_S_FEATURES_OK;
  R(VIRTIO_MMIO_STATUS) = status;
  if(!(R(VIRTIO_MMIO_STATUS) & VIRTIO_CONFIG_S_FEATURES_OK))
    panic("virtio disk FEATURES_OK unset");

  R(VIRTIO_MMIO_GUEST_PAGE_SIZE) = PGSIZE;
  R(VIRTIO_MMIO_QUEUE_SEL) = 0;
  uint32 max = R(VIRTIO_MMIO_QUEUE_NUM_MAX);
  if(max == 0) panic("virtio disk has no queue 0");
  if(max < 8) panic("virtio disk max queue too short");
  R(VIRTIO_MMIO_QUEUE_NUM) = 8;

  memset(disk.pages, 0, sizeof(disk.pages));
  R(VIRTIO_MMIO_QUEUE_PFN) = ((uint64)disk.pages) >> 12;

  disk.desc = (struct VRingDesc *) disk.pages;
  disk.avail = (uint16 *)(((char*)disk.desc) + 8*16);
  disk.used = (struct UsedArea *) (disk.pages + PGSIZE);

  for(int i = 0; i < 8; i++) disk.free[i] = 1;

  status |= VIRTIO_CONFIG_S_DRIVER_OK;
  R(VIRTIO_MMIO_STATUS) = status;
}

static int alloc_desc() {
  for(int i = 0; i < 8; i++){
    if(disk.free[i]){
      disk.free[i] = 0;
      return i;
    }
  }
  return -1;
}

static void free_desc(int i) {
  if(i >= 8) panic("free_desc 1");
  if(disk.free[i]) panic("free_desc 2");
  disk.desc[i].addr = 0;
  disk.desc[i].len = 0;
  disk.desc[i].flags = 0;
  disk.desc[i].next = 0;
  disk.free[i] = 1;
  wakeup(&disk.free); // wake up logic if needed
}

static void free_chain(int i) {
  while(1){
    int flag = disk.desc[i].flags;
    int nxt = disk.desc[i].next;
    free_desc(i);
    if(flag & VRING_DESC_F_NEXT)
      i = nxt;
    else
      break;
  }
}

static int alloc3_desc(int *idx) {
  for(int i = 0; i < 3; i++){
    idx[i] = alloc_desc();
    if(idx[i] < 0){
      for(int j = 0; j < i; j++)
        free_desc(idx[j]);
      return -1;
    }
  }
  return 0;
}

void virtio_disk_rw(struct buf *b, int write) {
  uint64 sector = b->blockno * (BSIZE / 512);

  acquire(&disk.vdisk_lock);

  int idx[3];
  while(1){
    if(alloc3_desc(idx) == 0) {
      break;
    }
    sleep(&disk.free, &disk.vdisk_lock);
  }

  struct virtio_blk_req {
    uint32 type;
    uint32 reserved;
    uint64 sector;
  } buf0;

  if(write) buf0.type = VIRTIO_BLK_T_OUT;
  else buf0.type = VIRTIO_BLK_T_IN;
  buf0.reserved = 0;
  buf0.sector = sector;

  disk.desc[idx[0]].addr = (uint64) &buf0;
  disk.desc[idx[0]].len = sizeof(buf0);
  disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
  disk.desc[idx[0]].next = idx[1];

  disk.desc[idx[1]].addr = (uint64) b->data;
  disk.desc[idx[1]].len = BSIZE;
  if(write) disk.desc[idx[1]].flags = 0;
  else disk.desc[idx[1]].flags = VRING_DESC_F_WRITE;
  disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
  disk.desc[idx[1]].next = idx[2];

  disk.info[idx[0]] = b;
  uint8 status;
  disk.desc[idx[2]].addr = (uint64) &status;
  disk.desc[idx[2]].len = 1;
  disk.desc[idx[2]].flags = VRING_DESC_F_WRITE;
  disk.desc[idx[2]].next = 0;

  b->disk = 1;
  disk.avail[2 + (disk.avail[1] % 8)] = idx[0];
  __sync_synchronize();
  disk.avail[1] = disk.avail[1] + 1;
  __sync_synchronize();
  R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

  while(b->disk == 1) {
    sleep(b, &disk.vdisk_lock);
  }
  disk.info[idx[0]] = 0;
  free_chain(idx[0]);
  release(&disk.vdisk_lock);
}

void virtio_disk_intr() {
  acquire(&disk.vdisk_lock);
  R(VIRTIO_MMIO_INTERRUPT_ACK) = R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;
  __sync_synchronize();

  while(disk.used_idx != disk.used->id){
    int id = disk.used->elems[disk.used_idx % 8].id;
    if(disk.info[id]) {
      struct buf *b = disk.info[id];
      b->disk = 0;
      wakeup(b);
    }
    disk.used_idx++;
  }
  release(&disk.vdisk_lock);
}