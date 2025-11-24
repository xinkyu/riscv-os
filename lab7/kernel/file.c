#include "defs.h"
#include "param.h"
#include "fs.h"
#include "file.h"
#include "stat.h"

struct devsw devsw[NDEV];

extern int consolewrite(int, uint64, int);
extern int consoleread(int, uint64, int);

struct {
    struct spinlock lock;
    struct file file[NFILE];
} ftable;

void fileinit(void) {
    spinlock_init(&ftable.lock, "ftable");
    devsw[CONSOLE].write = consolewrite;
    devsw[CONSOLE].read = consoleread;
}

struct file *filealloc(void) {
    acquire(&ftable.lock);
    for (struct file *f = ftable.file; f < ftable.file + NFILE; f++) {
        if (f->ref == 0) {
            f->ref = 1;
            release(&ftable.lock);
            return f;
        }
    }
    release(&ftable.lock);
    return 0;
}

struct file *filedup(struct file *f) {
    acquire(&ftable.lock);
    if (f->ref < 1) {
        panic("filedup");
    }
    f->ref++;
    release(&ftable.lock);
    return f;
}

void fileclose(struct file *f) {
    acquire(&ftable.lock);
    if (f->ref < 1) {
        panic("fileclose");
    }
    f->ref--;
    if (f->ref > 0) {
        release(&ftable.lock);
        return;
    }
    int type = f->type;
    struct inode *ip = f->ip;
    release(&ftable.lock);

    if (type == FD_INODE) {
        begin_op();
        iput(ip);
        end_op();
    }

    f->type = FD_NONE;
    f->readable = 0;
    f->writable = 0;
    f->ip = 0;
    f->off = 0;
    f->major = 0;
}

int filestat(struct file *f, uint64 addr) {
    if (f->type != FD_INODE) {
        return -1;
    }
    struct stat *st = (struct stat*)addr;
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
}

int fileread(struct file *f, uint64 addr, int n) {
    if (!f->readable) {
        return -1;
    }
    if (f->type == FD_INODE) {
        ilock(f->ip);
        int r = readi(f->ip, 0, addr, f->off, n);
        if (r > 0) {
            f->off += r;
        }
        iunlock(f->ip);
        return r;
    } else if (f->type == FD_DEVICE) {
        if (f->major < 0 || f->major >= NDEV || !devsw[f->major].read) {
            return -1;
        }
        return devsw[f->major].read(1, addr, n);
    }
    return -1;
}

int filewrite(struct file *f, uint64 addr, int n) {
    if (!f->writable) {
        return -1;
    }
    if (f->type == FD_DEVICE) {
        if (f->major < 0 || f->major >= NDEV || !devsw[f->major].write) {
            return -1;
        }
        return devsw[f->major].write(1, addr, n);
    }
    if (f->type != FD_INODE) {
        return -1;
    }

    int max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * BSIZE;
    int i = 0;
    while (i < n) {
        int n1 = n - i;
        if (n1 > max) {
            n1 = max;
        }
        begin_op();
        ilock(f->ip);
        int r = writei(f->ip, 0, addr + i, f->off, n1);
        if (r > 0) {
            f->off += r;
        }
        iunlock(f->ip);
        end_op();
        if (r < 0) {
            break;
        }
        if (r != n1) {
            panic("short filewrite");
        }
        i += r;
    }
    return i == n ? n : -1;
}
