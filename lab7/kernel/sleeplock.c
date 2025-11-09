// kernel/sleeplock.c
// 睡眠锁

#include "defs.h"
#include "sleeplock.h"
#include "proc.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{
  initspinlock(&lk->lk, "sleeplock"); // <-- 初始化内部的 spinlock
  lk->locked = 0;
  lk->name = name;
  lk->pid = 0;
}

void
acquiresleep(struct sleeplock *lk)
{
  acquire(&lk->lk); // <-- 获取 spinlock 来保护 locked 字段
  while (lk->locked) {
    // sleep 会原子性地释放 lk->lk 并进入睡眠
    sleep(lk, &lk->lk);
  }
  
  // 获取到锁
  lk->locked = 1;
  lk->pid = myproc()->pid;
  
  release(&lk->lk);
}

void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk); // <-- 获取 spinlock
  
  lk->locked = 0;
  lk->pid = 0;
  
  wakeup(lk); // 唤醒所有等待这个锁的进程
  
  release(&lk->lk); // <-- 释放 spinlock
}

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  acquire(&lk->lk);
  r = lk->locked && (lk->pid == myproc()->pid);
  release(&lk->lk);
  return r;
}