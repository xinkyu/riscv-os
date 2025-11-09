// kernel/file.h
#ifndef __FILE_H__
#define __FILE_H__

#include "fs.h"

struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // 引用计数
  char readable;
  char writable;
  struct pipe *pipe; // (实验后期)
  struct inode *ip;  // 指向 Inode
  uint off;          // 读写偏移量
};

// 内存中的 Inode (来自 fs.c)
extern struct inode icache[50]; // Inode 缓存

// devsw[].read 和 devsw[].write 的函数原型
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int either_copyin(void *dst, int user_src, uint64 src, uint64 len);

#endif // __FILE_H__