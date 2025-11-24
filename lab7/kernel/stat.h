#ifndef __STAT_H__
#define __STAT_H__

#include "riscv.h"

struct stat {
    short type;
    int dev;
    uint ino;
    short nlink;
    uint64 size;
};

#endif // __STAT_H__
