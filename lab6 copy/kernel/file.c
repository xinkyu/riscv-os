#include "defs.h"
#include "riscv.h"
#include "file.h"
#include "stat.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "param.h"

struct devsw devsw[NDEV];

struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void fileinit(void) {
  initlock(&ftable.lock, "ftable");
}

// 分配一个文件结构
struct file* filealloc(void) {
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// 增加文件引用计数 (用于 fork/dup)
struct file* filedup(struct file *f) {
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// 关闭文件
void fileclose(struct file *f) {
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE){
    // pipeclose(ff.pipe, ff.writable); // 暂不支持管道
  } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// 获取文件状态
int filestat(struct file *f, uint64 addr) {
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    ilock(f->ip);
    stati(f->ip, &st);
    iunlock(f->ip);
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// 读文件
int fileread(struct file *f, uint64 addr, int n) {
  int r = 0;

  if(f->readable == 0)
    return -1;

  if(f->type == FD_PIPE){
    // return piperead(f->pipe, addr, n);
    return -1;
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
      return -1;
    return devsw[f->major].read(1, addr, n);
  } else if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
    return r;
  }
  panic("fileread");
  return -1;
}

// 写文件
int filewrite(struct file *f, uint64 addr, int n) {
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_PIPE){
    // return pipewrite(f->pipe, addr, n);
    return -1;
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
      return -1;
    return devsw[f->major].write(1, addr, n);
  } else if(f->type == FD_INODE){
    // 写入 inode 需要事务保护
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE; // 简单限制单次写入大小，防止日志溢出
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r != n1){
        // 写入出错
        break;
      }
      i += r;
    }
    ret = (i == n ? n : -1);
  } else {
    panic("filewrite");
  }
  return ret;
}