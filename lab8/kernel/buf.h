#ifndef __BUF_H__
#define __BUF_H__

#include "sleeplock.h"
#include "spinlock.h"
#include "riscv.h"
#include "param.h"

struct buf {
    int valid;   // 数据是否有效
    int disk;    // 是否正在磁盘上读/写
    uint dev;    // 设备号
    uint blockno;// 块号
    struct sleeplock lock;
    uint refcnt;
    struct buf *prev; // LRU 双向链表
    struct buf *next;
    uchar data[BSIZE];
};

#endif // __BUF_H__
