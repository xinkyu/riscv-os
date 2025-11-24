#ifndef __FILE_H__
#define __FILE_H__

#include "fs.h"

#define FD_NONE   0
#define FD_INODE  1
#define FD_DEVICE 2

struct file {
    int type;
    int ref;
    char readable;
    char writable;
    struct inode *ip;
    uint off;
    short major;
};

struct devsw {
    int (*read)(int, uint64, int);
    int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1

#endif // __FILE_H__
