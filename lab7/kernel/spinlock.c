// kernel/spinlock.c
// 互斥自旋锁

#include "defs.h"
#include "spinlock.h"

// RISC-V 的原子交换指令 (atomic memory operation - swap, word)
static inline uint
atomic_swap(volatile uint *addr, uint val)
{
    int res;
    // amoswap.w.aq rd, rs1, (rs2)
    // .aq (acquire) 保证在交换前的所有内存操作不会被重排到交换之后
    asm volatile(
        "amoswap.w.aq %0, %1, (%2)"
        : "=r"(res)
        : "r"(val), "r"(addr)
        : "memory"
    );
    return res;
}

// 初始化自旋锁
void
initspinlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// 获取自旋锁
void
acquire(struct spinlock *lk)
{
  intr_off(); // 关中断
  if(holding(lk)) {
    printf("acquire: lock %s already held\n", lk->name);
    while(1); // panic
  }
  
  // 循环尝试，直到 lk->locked 从 0 变为 1
  while(atomic_swap(&lk->locked, 1) != 0)
    ;

  // 记录调试信息
  lk->cpu = mycpu();
}

// 释放自旋锁
void
release(struct spinlock *lk)
{
  if(!holding(lk)) {
    printf("release: lock %s not held\n", lk->name);
    while(1); // panic
  }

  lk->cpu = 0;

  // amoswap.w.rl (release)
  // .rl 保证在交换后的所有内存操作不会被重排到交换之前
  asm volatile(
      "amoswap.w.rl zero, zero, (%0)"
      :
      : "r"(&lk->locked)
      : "memory"
  );

  intr_on(); // 开中断
}

// 检查当前CPU是否持有该锁
int
holding(struct spinlock *lk)
{
  int r;
  push_off();
  r = lk->locked && lk->cpu == mycpu();
  pop_off();
  return r;
}

//
// push_off/pop_off 用于临时关闭/开启中断
//
void
push_off(void)
{
  intr_off();
}

void
pop_off(void)
{
  intr_on();
}