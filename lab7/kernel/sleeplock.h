// kernel/sleeplock.h
#ifndef __SLEEPLOCK_H__
#define __SLEEPLOCK_H__

#include "types.h"    // <-- 包含 types.h
#include "spinlock.h" // <-- 包含 spinlock.h

// 长时间持有的锁
struct sleeplock {
  uint locked;       // 锁是否被持有 (0 or 1)
  struct spinlock lk; // 保护这个 sleeplock 状态的自旋锁
  
  // 用于调试
  char *name;        // 锁的名称
  int pid;           // 持有锁的进程PID
};

#endif // __SLEEPLOCK_H__