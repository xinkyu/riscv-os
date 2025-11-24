#ifndef __BUF_H__
#define __BUF_H__

#include "spinlock.h"
#include "fs.h" // 包含 BSIZE
#include "sleeplock.h"
struct buf {
  int valid;   // 数据是否已从磁盘读取
  int disk;    // 是否已被修改需要写回磁盘
  uint dev;    // 设备号
  uint blockno;// 块号
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU 链表
  struct buf *next;
  uchar data[BSIZE];
};

#endif