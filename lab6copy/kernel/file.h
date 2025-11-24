#ifndef __FILE_H__
#define __FILE_H__

#include "sleeplock.h"
#include "fs.h"

struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // 引用计数
  char readable;
  char writable;
  struct pipe *pipe; // FD_PIPE
  struct inode *ip;  // FD_INODE and FD_DEVICE
  uint off;          // FD_INODE offset
  short major;       // FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define mkdev(m,n)  ((uint)((m)<<16| (n)))

// 内存中的 inode 副本
struct inode {
  uint dev;           // 设备号
  uint inum;          // Inode 编号
  int ref;            // 引用计数
  struct sleeplock lock; // 保护以下字段
  int valid;          // 是否已从磁盘读取

  short type;         // 磁盘 inode 的副本
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1];
};

// map major device number to device functions.
struct devsw {
  int (*read)(int, uint64, int);
  int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1

#endif