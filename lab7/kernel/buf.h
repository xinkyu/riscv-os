// kernel/buf.h
#ifndef __BUF_H__
#define __BUF_H__

#include "proc.h" // sleeplock 需要 proc.h

#define BSIZE 1024 // 块大小 (Block size)

struct buf {
  int valid;   // 缓存中的数据是否已从磁盘读取
  int disk;    // buf 是否代表一个有效的磁盘块
  uint dev;
  uint blockno;
  struct sleeplock lock; // 保护每个 buf 的锁
  uint refcnt;  // 引用计数
  struct buf *prev; // LRU 链表
  struct buf *next;
  uchar data[BSIZE];
};

#endif // __BUF_H__