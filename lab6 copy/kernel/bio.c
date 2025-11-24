#include "defs.h"
#include "riscv.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[30]; // 缓存 30 个块，对应指南建议的 LOG_SIZE 30
  struct buf head;
} bcache;

void binit(void) {
  struct buf *b;
  spinlock_init(&bcache.lock, "bcache");

  // 初始化 LRU 双向链表
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf + 30; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    spinlock_init(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// 获取缓存块（如果不在缓存中则分配）
static struct buf* bget(uint dev, uint blockno) {
  struct buf *b;

  acquire(&bcache.lock);

  // 1. 检查是否已缓存
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 2. 分配新的缓存块 (LRU)
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  panic("bget: no buffers");
  return 0;
}

// 读取磁盘块
struct buf* bread(uint dev, uint blockno) {
  struct buf *b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0); // 0 = read
    b->valid = 1;
  }
  return b;
}

// 写入磁盘块
void bwrite(struct buf *b) {
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1); // 1 = write
}

// 释放缓存块
void brelse(struct buf *b) {
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // 移到链表头部 (LRU)
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  release(&bcache.lock);
}

void bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}