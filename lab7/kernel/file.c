// kernel/file.c
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"

struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void fileinit(void) {
  initlock(&ftable.lock, "ftable");
}

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

void fileclose(struct file *f) {
  struct file ff;
  acquire(&ftable.lock);
  if(f->ref < 1) panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_INODE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

int fileread(struct file *f, uint64 addr, int n) {
  int r = 0;
  if(f->readable == 0) return -1;
  if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
  }
  return r;
}

int filewrite(struct file *f, uint64 addr, int n) {
  int r = 0;
  if(f->writable == 0) return -1;
  if(f->type == FD_INODE){
    // max file size checks...
    begin_op();
    ilock(f->ip);
    if((r = writei(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iupdate(f->ip);
    iunlock(f->ip);
    end_op();
  }
  return r;
}