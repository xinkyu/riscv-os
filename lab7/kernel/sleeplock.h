#ifndef __SLEEPLOCK_H__
#define __SLEEPLOCK_H__

#include "spinlock.h"

struct proc;

struct sleeplock {
    uint locked;       // 是否持有
    struct spinlock lk;// 自旋锁保护 sleeplock
    char *name;
    struct proc *owner;
};

void initsleeplock(struct sleeplock *lk, char *name);
void acquiresleep(struct sleeplock *lk);
void releasesleep(struct sleeplock *lk);
int holdingsleep(struct sleeplock *lk);

#endif // __SLEEPLOCK_H__
