#ifndef __DEFS_H__
#define __DEFS_H__

#include "riscv.h"
#include "proc.h"

struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct spinlock;
struct sleeplock;
struct stat;
struct superblock;

// bio.c
void            binit(void);
struct buf* bread(uint, uint);
void            brelse(struct buf*);
void            bwrite(struct buf*);
void            bpin(struct buf*);
void            bunpin(struct buf*);

// console.c
void            consoleinit(void);
void            cons_putc(char c);
void            consoleintr(int);

// file.c
struct file* filealloc(void);
void            fileclose(struct file*);
struct file* filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, uint64, int n);
int             filestat(struct file*, uint64 addr);
int             filewrite(struct file*, uint64, int n);

// fs.c
void            fsinit(int);
int             dirlink(struct inode*, char*, uint);
struct inode* dirlookup(struct inode*, char*, uint*);
struct inode* ialloc(uint, short);
struct inode* idup(struct inode*);
void            iinit();
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode* namei(char*);
struct inode* nameiparent(char*, char*);
int             readi(struct inode*, int, uint64, uint, uint);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, int, uint64, uint, uint);
void            itrunc(struct inode*);

// kalloc.c
void            kinit();
void            freerange(void *pa_start, void *pa_end);
void            kfree(void *pa);
void* kalloc(void);

// log.c
void            initlog(int, struct superblock*);
void            log_write(struct buf*);
void            begin_op(void);
void            end_op(void);

// printf.c
void            printf(const char *fmt, ...);
void            clear_screen();
void            panic(char*) __attribute__((noreturn));

// proc.c
int             fork(void);           
void            exit(int);
int             wait(int*);
void            sleep(void*, struct spinlock*);
void            wakeup(void*);
void            yield(void);
int             create_process(void (*entry)(void));
void            procinit(void);
void            scheduler(void) __attribute__((noreturn));
struct proc* myproc(void);
struct cpu* mycpu(void);
void            wait_process(int*);
struct proc* get_current_proc_debug(void);

// string.c
int             memcmp(const void*, const void*, uint);
void* memmove(void*, const void*, uint);
void* memset(void*, int, uint);
char* safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char* strncpy(char*, const char*, int);

// swtch.S
void            swtch(struct context *old, struct context *new);

// spinlock.c
void            spinlock_init(struct spinlock *lk, char *name);
void            acquire(struct spinlock *lk);
void            release(struct spinlock *lk);
void            push_off(void);
void            pop_off(void);

// 兼容性宏：允许 log.c/fs.c 使用 initlock 调用 spinlock_init
#define initlock spinlock_init

// syscall.c 
void            syscall(void);
int             argint(int n, int *ip);
int             argaddr(int n, uint64 *ip);
int             argstr(int n, char *buf, int max);

// sysfile.c 
int             sys_read(void);
int             sys_write(void);
int             sys_open(void);
int             sys_close(void);
int             sys_dup(void);
int             sys_fstat(void);
int             sys_link(void);
int             sys_unlink(void);
int             sys_mkdir(void);
int             sys_chdir(void);
int             sys_pipe(void);
int             sys_mknod(void);
// 暴露给内核测试代码使用
struct inode* create(char *path, short type, short major, short minor);

// sysproc.c 
int             sys_exit(void);
int             sys_fork(void);
int             sys_wait(void);
int             sys_kill(void);
int             sys_getpid(void);

// trap.c
void            trap_init(void);
void            clock_init(void);
uint64          get_time(void);
uint64          get_interrupt_count(void);
uint64          get_ticks(void);
void* get_ticks_channel(void);
void            kerneltrap(uint64);

// uart.c
void            uartinit(void);
void            uart_putc(char c);
void            uartintr(void);

// virtio_disk.c
void            virtio_disk_init(void);
void            virtio_disk_rw(struct buf *b, int write);
void            virtio_disk_intr(void);

// vm.c
void            kvminit(void);
void            kvminithart(void);
pagetable_t     create_pagetable(void);
int             map_page(pagetable_t pt, uint64 va, uint64 pa, int perm);
pte_t* walk_lookup(pagetable_t pt, uint64 va);
int             copyout(pagetable_t, uint64, char *, uint64);
int             copyin(pagetable_t, char *, uint64, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);

// sleeplock.c
void            initsleeplock(struct sleeplock*, char*);
void            acquiresleep(struct sleeplock*);
void            releasesleep(struct sleeplock*);
int             holdingsleep(struct sleeplock*);

#endif // __DEFS_H__