// kernel/defs.h
#ifndef __DEFS_H__
#define __DEFS_H__

#include "riscv.h"
#include "proc.h"
#include "types.h"
// console.c
void cons_putc(char c);

// printf.c
void printf(const char *fmt, ...);
void clear_screen();

// uart.c
void uart_putc(char c);

// kalloc.c
void kinit();
void freerange(void *pa_start, void *pa_end);
void kfree(void *pa);
void *kalloc(void);

// vm.c
void kvminit(void);
void kvminithart(void);
pagetable_t create_pagetable(void);
int map_page(pagetable_t pt, uint64 va, uint64 pa, int perm);
pte_t *walk_lookup(pagetable_t pt, uint64 va);

// trap.c
void trap_init(void);
void clock_init(void);
uint64 get_time(void);
uint64 get_interrupt_count(void);
uint64 get_ticks(void);
void* get_ticks_channel(void);

// proc.c
int  fork(void);           
void exit(int);
int  wait(int*);
void sleep(void*, struct spinlock*);
void wakeup(void*);
void yield(void);
int  create_process(void (*entry)(void));
void procinit(void);
void scheduler(void) __attribute__((noreturn));
struct proc* myproc(void);
struct cpu* mycpu(void);
void wait_process(int*);

// [修复] 必须在这里声明调试接口，test.c 才能看见它
struct proc* get_current_proc_debug(void); 

// syscall.c 
void syscall(void);
int  argint(int n, int *ip);
int  argaddr(int n, uint64 *ip);
int  argstr(int n, char *buf, int max);

// sysproc.c 
int  sys_exit(void);
int  sys_fork(void);
int  sys_wait(void);
int  sys_kill(void);
int  sys_getpid(void);

// sysfile.c 
int  sys_read(void);
int  sys_write(void);
int  sys_open(void);
int  sys_close(void);

// swtch.S
void swtch(struct context *old, struct context *new);

// spinlock.c
void spinlock_init(struct spinlock *lk, char *name);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
void push_off(void);
void pop_off(void);

// test.c
void run_all_tests(void);
void run_lab4_tests(void);
void run_lab5_tests(void);
void run_lab6_tests(void); 
// ... 添加 ...

// bio.c
void binit(void);
struct buf* bread(uint, uint);
void brelse(struct buf*);
void bwrite(struct buf*);

// log.c
void log_init(int, struct superblock*);
void begin_op(void);
void end_op(void);
void log_write(struct buf*);

// fs.c
void iinit(void);
struct inode* iget(uint, uint);
void ilock(struct inode*);
void iunlock(struct inode*);
void iunlockput(struct inode*);
void iput(struct inode*);
void iupdate(struct inode*);
int namecmp(const char*, const char*);
struct inode* namei(char*);
struct inode* dirlookup(struct inode*, char*, uint*);
int dirlink(struct inode*, char*, uint);
int readi(struct inode*, int, uint64, uint, uint);
int writei(struct inode*, int, uint64, uint, uint);
void read_sb(int, struct superblock*);

// file.c
void fileinit(void);
struct file* filealloc(void);
void fileclose(struct file*);
int fileread(struct file*, uint64, int);
int filewrite(struct file*, uint64, int);

// virtio_disk.c
void virtio_disk_init(void);
void virtio_disk_rw(struct buf*, int);
void virtio_disk_intr(void);

// sysfile.c
int sys_unlink(void);

// string.c
int memcmp(const void*, const void*, uint);
void* memmove(void*, const void*, uint);
void* memset(void*, int, uint);
char* safestrcpy(char*, const char*, int);
int strlen(const char*);
int strncmp(const char*, const char*, uint);
char* strncpy(char*, const char*, int);

// plic.c
void plic_init(void);
void plic_inithart(void);
int plic_claim(void);
void plic_complete(int);
#endif // __DEFS_H__