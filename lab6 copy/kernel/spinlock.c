// kernel/spinlock.c
#include "defs.h"

void spinlock_init(struct spinlock *lk, char *name) {
    lk->name = name;
    lk->locked = 0;
    lk->cpu = 0;
}

// 获取锁
// 必须屏蔽中断以避免死锁
void acquire(struct spinlock *lk) {
    push_off(); // 屏蔽中断
    if (lk->locked && lk->cpu == mycpu()) {
        printf("acquire: re-acquire lock %s\n", lk->name);
        while(1);
    }

    // 使用 RISC-V 的 'amst' (atomic swap) 指令
    while (__atomic_test_and_set(&lk->locked, __ATOMIC_ACQUIRE));

    // 记录持有锁的CPU，用于调试
    lk->cpu = mycpu();
}

// 释放锁
void release(struct spinlock *lk) {
    if (!lk->locked) {
        printf("release: lock %s not held\n", lk->name);
        while(1);
    }

    lk->cpu = 0;
    __atomic_clear(&lk->locked, __ATOMIC_RELEASE);
    pop_off(); // 恢复之前的中断状态
}

// --- 中断状态保存 ---

// 记录 push_off/pop_off 的嵌套层数
void push_off(void) {
    int old = intr_get();
    intr_off();
    if (mycpu()->ncli == 0) {
        mycpu()->intena = old;
    }
    mycpu()->ncli += 1;
}

void pop_off(void) {
    if (intr_get()) {
        printf("pop_off: interrupts enabled\n");
        while(1);
    }
    mycpu()->ncli -= 1;
    if (mycpu()->ncli < 0) {
        printf("pop_off: ncli < 0\n");
        while(1);
    }
    if (mycpu()->ncli == 0 && mycpu()->intena) {
        intr_on();
    }
}