// kernel/spinlock.h (修正版 v3)
#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "riscv.h" // 包含它以获取 uint64 的定义

// 自旋锁
struct spinlock {
    uint64 locked;
    char *name;
    struct cpu *cpu;
};

void spinlock_init(struct spinlock *lk, char *name);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);

void push_off(void);
void pop_off(void);

// 内联函数
static inline void intr_off() {
    w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

static inline void intr_on() {
    w_sstatus(r_sstatus() | SSTATUS_SIE);
}

// 修正: 添加回缺失的 intr_get()
static inline int intr_get(void) {
    return (r_sstatus() & SSTATUS_SIE) ? 1 : 0;
}

#endif // __SPINLOCK_H__