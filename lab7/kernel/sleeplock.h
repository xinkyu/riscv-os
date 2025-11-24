#ifndef __SLEEPLOCK_H__
#define __SLEEPLOCK_H__

#include "spinlock.h"

struct sleeplock {
  uint locked;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock
  
  // For debugging:
  char *name;        // Name of lock.
  int pid;           // Process holding lock
};

#endif