#include "defs.h"
#include "sleeplock.h"

void initsleeplock(struct sleeplock *lk, char *name) {
    spinlock_init(&lk->lk, "sleeplock");
    lk->name = name;
    lk->locked = 0;
    lk->owner = 0;
}

void acquiresleep(struct sleeplock *lk) {
    acquire(&lk->lk);
    while (lk->locked) {
        sleep(lk, &lk->lk);
    }
    lk->locked = 1;
    lk->owner = myproc();
    release(&lk->lk);
}

void releasesleep(struct sleeplock *lk) {
    acquire(&lk->lk);
    lk->locked = 0;
    lk->owner = 0;
    wakeup(lk);
    release(&lk->lk);
}

int holdingsleep(struct sleeplock *lk) {
    int r;

    acquire(&lk->lk);
    r = lk->locked && lk->owner == myproc();
    release(&lk->lk);
    return r;
}
