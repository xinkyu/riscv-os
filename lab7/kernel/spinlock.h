// kernel/spinlock.h
#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "types.h" // <-- 包含 types.h
#include "riscv.h" // <-- 包含 riscv.h

struct cpu; // <-- 前向声明：告诉编译器 "struct cpu" 是一个存在的东西

// 互斥自旋锁
struct spinlock {
  uint locked;       // 锁是否被持有

  // 用于调试
  char *name;        // 锁的名称
  struct cpu *cpu;   // 持有锁的CPU (现在编译器认识 'struct cpu *' 了)
};

#endif // __SPINLOCK_H__