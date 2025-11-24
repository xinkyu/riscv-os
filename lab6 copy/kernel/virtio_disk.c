#include "defs.h"
#include "riscv.h"
#include "fs.h"
#include "buf.h"
#include "virtio.h"

// 磁盘驱动描述符结构
struct disk {
  // 描述符数组：设备使用这些描述符来告诉驱动哪个缓冲区可以使用
  struct virtq_desc *desc;
  // 可用环：驱动告诉设备哪些描述符已准备好
  struct virtq_avail *avail;
  // 已用环：设备告诉驱动哪些请求已完成
  struct virtq_used *used;

  // 我们的记录，用于追踪正在进行的请求
  char free[8];  // 描述符是否空闲
  uint16 used_idx; // 我们上次检查的 used 环索引

  // 正在进行的每个描述符对应的 buffer info
  struct {
    struct buf *b;
    char status;
  } info[8];

  // 磁盘命令头 (VirtIO 协议要求)
  struct virtio_blk_req {
    uint32 type;
    uint32 reserved;
    uint64 sector;
  } ops[8];
  
  struct spinlock vdisk_lock;
} disk;

void virtio_disk_init(void) {
  uint32 status = 0;

  spinlock_init(&disk.vdisk_lock, "virtio_disk");

  if(*(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
     *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_VERSION) != 1 ||
     *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_DEVICE_ID) != 2 ||
     *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_VENDOR_ID) != 0x554d4551){
    panic("could not find virtio disk");
  }
  
  status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_STATUS) = status;

  status |= VIRTIO_CONFIG_S_DRIVER;
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_STATUS) = status;

  // 协商特性
  uint64 features = *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_DEVICE_FEATURES);
  features &= ~(1 << VIRTIO_BLK_F_RO);
  features &= ~(1 << VIRTIO_BLK_F_SCSI);
  features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
  features &= ~(1 << VIRTIO_BLK_F_MQ);
  features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
  features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
  features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_DRIVER_FEATURES) = features;

  status |= VIRTIO_CONFIG_S_FEATURES_OK;
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_STATUS) = status;

  status |= VIRTIO_CONFIG_S_DRIVER_OK;
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_STATUS) = status;

  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_SEL) = 0;
  if(*(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_READY))
    panic("virtio disk should not be ready");

  uint32 max = *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_NUM_MAX);
  if(max == 0) panic("virtio disk has no queue 0");
  if(max < 8) panic("virtio disk max queue too short");

  // 分配并映射两个页用于 virtio 环
  // 这里简化处理，直接使用 kalloc 分配连续物理内存
  // 注意：在实际复杂系统中，需要确保这些页是对齐且连续的
  struct virtq_desc *pages = kalloc(); 
  kalloc(); // 这里其实需要保证连续，简单实现中通常 kalloc 只是递增指针，但在分页系统中需要小心
  // 为了稳健，我们应该在 kernel/vm.c 中增加一个分配连续页的接口，或者只用 1 页 (4KB 足够存放 8 个描述符)
  // 标准 xv6 使用 4096 字节足够存放 QueueNum=8 的所有结构
  
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_NUM) = 8;
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)pages;
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)pages >> 32;
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_DRIVER_LOW) = (uint64)((char*)pages + 0x200); // 偏移量需计算
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_DRIVER_HIGH) = (uint64)((char*)pages + 0x200) >> 32;
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_DEVICE_LOW) = (uint64)((char*)pages + 0x300); // 偏移量需计算
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_DEVICE_HIGH) = (uint64)((char*)pages + 0x300) >> 32;

  // 正确设置指针
  disk.desc = pages;
  disk.avail = (struct virtq_avail *)((char*)pages + 0x200); // 简单硬编码偏移
  disk.used = (struct virtq_used *)((char*)pages + 0x300);

  for(int i = 0; i < 8; i++) disk.free[i] = 1;

  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_READY) = 1;
}

// 分配描述符
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
  wakeup(&disk.free[0]);
}

void virtio_disk_rw(struct buf *b, int write) {
  uint64 sector = b->blockno * (BSIZE / 512);

  acquire(&disk.vdisk_lock);

  int idx[3];
  while(1){
    if((idx[0] = alloc_desc()) >= 0){
      if((idx[1] = alloc_desc()) >= 0){
        if((idx[2] = alloc_desc()) >= 0){
          break;
        }
        free_desc(idx[1]);
      }
      free_desc(idx[0]);
    }
    sleep(&disk.free[0], &disk.vdisk_lock);
  }

  // 格式化 virtio 请求
  struct virtio_blk_req *buf0 = &disk.ops[idx[0]];
  buf0->type = write ? 2 : 1; // VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN
  buf0->reserved = 0;
  buf0->sector = sector;

  disk.desc[idx[0]].addr = (uint64) buf0;
  disk.desc[idx[0]].len = sizeof(struct virtio_blk_req);
  disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
  disk.desc[idx[0]].next = idx[1];

  disk.desc[idx[1]].addr = (uint64) b->data;
  disk.desc[idx[1]].len = BSIZE;
  disk.desc[idx[1]].flags = write ? 0 : VRING_DESC_F_WRITE; 
  disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
  disk.desc[idx[1]].next = idx[2];

  disk.info[idx[0]].status = 0xff; // 设备写入 0 表示成功
  disk.desc[idx[2]].addr = (uint64) &disk.info[idx[0]].status;
  disk.desc[idx[2]].len = 1;
  disk.desc[idx[2]].flags = VRING_DESC_F_WRITE;
  disk.desc[idx[2]].next = 0;

  // 记录属于哪个 buffer
  b->disk = 1; 
  disk.info[idx[0]].b = b;

  // 将描述符链首放入 avail 环
  disk.avail->ring[disk.avail->idx % 8] = idx[0];
  __sync_synchronize();
  disk.avail->idx += 1;
  __sync_synchronize();
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // Queue 0

  // 等待完成
  while(b->disk == 1) {
    sleep(b, &disk.vdisk_lock);
  }

  disk.info[idx[0]].b = 0;
  free_desc(idx[0]);
  free_desc(idx[1]);
  free_desc(idx[2]);

  release(&disk.vdisk_lock);
}

// 磁盘中断处理
void virtio_disk_intr() {
  acquire(&disk.vdisk_lock);
  
  // 确认中断
  *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_INTERRUPT_ACK) = 
    *(volatile uint32 *)(VIRTIO0 + VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;
  __sync_synchronize();

  while(disk.used_idx != disk.used->idx){
    __sync_synchronize();
    int id = disk.used->ring[disk.used_idx % 8].id;

    if(disk.info[id].status != 0)
      panic("virtio_disk_intr status");

    struct buf *b = disk.info[id].b;
    b->disk = 0;   // 磁盘操作完成
    wakeup(b);

    disk.used_idx += 1;
  }
  release(&disk.vdisk_lock);
}