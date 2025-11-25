// kernel/sysfile.c
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "file.h"
#include "stat.h"
#include "fcntl.h"
#include "riscv.h" // for PTE_V

extern pagetable_t kernel_pagetable;

static int validate_addr(uint64 va, int len) {
    if (len < 0) return 0;
    uint64 start = va;
    uint64 end = va + len;
    if (end < start) return 0;

    for (uint64 a = PGROUNDDOWN(start); a < end; a += PGSIZE) {
        pte_t *pte = walk_lookup(kernel_pagetable, a);
        if (pte == 0 || (*pte & PTE_V) == 0) {
            return 0;
        }
    }
    return 1;
}

static struct inode *create(char *path, short type, short major, short minor) {
    struct inode *ip, *dp;
    char name[DIRSIZ];

    // DEBUG PRINT
    // printf("create: path=%s type=%d\n", path, type);

    if ((dp = nameiparent(path, name)) == 0) {
        printf("create: nameiparent failed for %s\n", path); // DEBUG
        return 0;
    }

    ilock(dp);

    if ((ip = dirlookup(dp, name, 0)) != 0) {
        iunlockput(dp);
        ilock(ip);
        if (type == T_FILE && ip->type == T_FILE)
            return ip;
        iunlockput(ip);
        return 0;
    }

    if ((ip = ialloc(dp->dev, type)) == 0)
        panic("create: ialloc");

    ilock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    iupdate(ip);

    if (type == T_DIR) {
        dp->nlink++;  
        iupdate(dp);
        if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
            panic("create dots");
    }

    if (dirlink(dp, name, ip->inum) < 0)
        panic("create: dirlink");

    iunlockput(dp);

    return ip;
}

static int argfd(int n, int *pfd, struct file **pf) {
    int fd;
    struct file *f;
    if (argint(n, &fd) < 0)
        return -1;
    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == 0)
        return -1;
    if (pfd)
        *pfd = fd;
    if (pf)
        *pf = f;
    return 0;
}

static int fdalloc(struct file *f) {
    struct proc *p = myproc();
    for (int fd = 0; fd < NOFILE; fd++) {
        if (p->ofile[fd] == 0) {
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

int sys_dup(void) {
    struct file *f;
    if (argfd(0, 0, &f) < 0)
        return -1;
    int fd = fdalloc(f);
    if (fd < 0)
        return -1;
    filedup(f);
    return fd;
}

int sys_read(void) {
    struct file *f;
    uint64 p;
    int n;
    if (argfd(0, 0, &f) < 0 || argaddr(1, &p) < 0 || argint(2, &n) < 0)
        return -1;
    return fileread(f, p, n);
}

int sys_write(void) {
    struct file *f;
    uint64 p;
    int n;
    int fd;

    if (argint(0, &fd) < 0) return -1;

    if ((fd == 1 || fd == 2) && myproc()->ofile[fd] == 0) {
        if (argaddr(1, &p) < 0 || argint(2, &n) < 0) return -1;
        if (!validate_addr(p, n)) return -1;

        char *s = (char*)p;
        for(int i = 0; i < n; i++) {
            cons_putc(s[i]);
        }
        return n;
    }

    if (argfd(0, 0, &f) < 0 || argaddr(1, &p) < 0 || argint(2, &n) < 0)
        return -1;
    
    if (!validate_addr(p, n)) {
        return -1;
    }

    return filewrite(f, p, n);
}

int sys_close(void) {
    int fd;
    struct file *f;
    if (argfd(0, &fd, &f) < 0)
        return -1;
    myproc()->ofile[fd] = 0;
    fileclose(f);
    return 0;
}

int sys_fstat(void) {
    struct file *f;
    uint64 addr;
    if (argfd(0, 0, &f) < 0 || argaddr(1, &addr) < 0)
        return -1;
    return filestat(f, addr);
}

int sys_link(void) {
    char name[DIRSIZ], new[MAXPATH], old[MAXPATH];
    struct inode *dp, *ip;

    if (argstr(0, old, sizeof(old)) < 0 || argstr(1, new, sizeof(new)) < 0)
        return -1;

    begin_op();
    if ((ip = namei(old)) == 0) {
        end_op();
        return -1;
    }
    ilock(ip);
    if (ip->type == T_DIR) {
        iunlockput(ip);
        end_op();
        return -1;
    }

    ip->nlink++;
    iupdate(ip);
    iunlock(ip);

    if ((dp = nameiparent(new, name)) == 0)
        goto bad;
    ilock(dp);
    if (dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0) {
        iunlockput(dp);
        goto bad;
    }
    iunlockput(dp);
    iput(ip);
    end_op();
    return 0;

bad:
    ilock(ip);
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    end_op();
    return -1;
}

int sys_unlink(void) {
    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ], path[MAXPATH];
    uint off;

    if (argstr(0, path, sizeof(path)) < 0)
        return -1;

    begin_op();
    if ((dp = nameiparent(path, name)) == 0) {
        end_op();
        return -1;
    }
    ilock(dp);

    if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
        goto bad;

    if ((ip = dirlookup(dp, name, &off)) == 0)
        goto bad;
    ilock(ip);

    if (ip->nlink < 1)
        panic("unlink: nlink < 1");
    if (ip->type == T_DIR && ip->size > 2 * sizeof(de)) {
        iunlockput(ip);
        goto bad;
    }

    memset(&de, 0, sizeof(de));
    if (writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
        panic("unlink: write");
    if (ip->type == T_DIR) {
        dp->nlink--;
        iupdate(dp);
    }
    iunlockput(dp);

    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);

    end_op();
    return 0;

bad:
    iunlockput(dp);
    end_op();
    return -1;
}

int sys_open(void) {
    char path[MAXPATH];
    int omode;
    struct file *f;
    struct inode *ip;

    if (argstr(0, path, sizeof(path)) < 0 || argint(1, &omode) < 0)
        return -1;

    begin_op();

    if (omode & O_CREATE) {
        ip = create(path, T_FILE, 0, 0);
        if (ip == 0) {
            printf("sys_open: create failed for %s\n", path); // DEBUG
            end_op();
            return -1;
        }
    } else {
        if ((ip = namei(path)) == 0) {
            end_op();
            return -1;
        }
        ilock(ip);
        if (ip->type == T_DIR && omode != O_RDONLY) {
            iunlockput(ip);
            end_op();
            return -1;
        }
    }

    if ((f = filealloc()) == 0) {
        iunlockput(ip);
        end_op();
        return -1;
    }
    int fd = fdalloc(f);
    if (fd < 0) {
        fileclose(f);
        iunlockput(ip);
        end_op();
        return -1;
    }
    if (ip->type == T_DEV) {
        f->type = FD_DEVICE;
        f->major = ip->major;
    } else {
        f->type = FD_INODE;
    }
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & (O_WRONLY | O_RDWR)) != 0;
    f->ip = ip;
    if (omode & O_TRUNC) {
        itrunc(ip);
    }
    iunlock(ip);
    end_op();
    return fd;
}

int sys_mkdir(void) {
    char path[MAXPATH];
    if (argstr(0, path, sizeof(path)) < 0)
        return -1;
    begin_op();
    struct inode *ip = create(path, T_DIR, 0, 0);
    if (ip == 0) {
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

int sys_mknod(void) {
    char path[MAXPATH];
    int major, minor;
    if (argstr(0, path, sizeof(path)) < 0 || argint(1, &major) < 0 || argint(2, &minor) < 0)
        return -1;
    begin_op();
    struct inode *ip = create(path, T_DEV, major, minor);
    if (ip == 0) {
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

int sys_chdir(void) {
    char path[MAXPATH];
    struct inode *ip;
    if (argstr(0, path, sizeof(path)) < 0)
        return -1;
    begin_op();
    if ((ip = namei(path)) == 0) {
        end_op();
        return -1;
    }
    ilock(ip);
    if (ip->type != T_DIR) {
        iunlockput(ip);
        end_op();
        return -1;
    }
    iunlock(ip);
    struct proc *p = myproc();
    iput(p->cwd);
    p->cwd = ip;
    end_op();
    return 0;
}