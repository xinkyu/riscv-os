#include "defs.h"
#include "riscv.h"
#include "fs.h"
#include "buf.h"

// 简单的日志结构：
// header block + data blocks

struct logheader {
  int n;
  int block[30]; // LOGSIZE
};

struct log {
  struct spinlock lock;
  int start;
  int size;
  int outstanding; // 正在进行的系统调用数量
  int committing;  // 是否正在提交
  int dev;
  struct logheader lh;
};

struct log log;

static void recover_from_log(void);
static void commit();

void initlog(int dev, struct superblock *sb) {
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  initlock(&log.lock, "log");
  log.start = sb->logstart;
  log.size = sb->nlog;
  log.dev = dev;
  recover_from_log();
}

// 将日志块从磁盘日志区复制到其真正的目标位置
static void install_trans(int recovering) {
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start + tail + 1); // 读取日志块
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]);   // 读取目标块
    memmove(dbuf->data, lbuf->data, BSIZE);
    bwrite(dbuf);  // 写回目标位置
    if(recovering == 0)
      bunpin(dbuf);
    brelse(lbuf);
    brelse(dbuf);
  }
}

// 读取日志头
static void read_head(void) {
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// 写入日志头
static void write_head(void) {
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  lh->n = log.lh.n;
  for (i = 0; i < log.lh.n; i++) {
    lh->block[i] = log.lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void recover_from_log(void) {
  read_head();
  install_trans(1); // 如果日志头里有记录，说明上次崩溃了，需要重放
  log.lh.n = 0;
  write_head(); // 清空日志
}

// 开始一个文件系统操作
void begin_op(void) {
  acquire(&log.lock);
  while(1){
    if(log.committing){
      sleep(&log, &log.lock);
    } else if(log.lh.n + (log.outstanding + 1)*5 > log.size){
      // 空间不够，等待当前事务提交
      sleep(&log, &log.lock);
    } else {
      log.outstanding += 1;
      release(&log.lock);
      break;
    }
  }
}

// 结束一个文件系统操作
void end_op(void) {
  int do_commit = 0;

  acquire(&log.lock);
  log.outstanding -= 1;
  if(log.committing)
    panic("log.committing");
  if(log.outstanding == 0){
    do_commit = 1;
    log.committing = 1;
  } else {
    // 唤醒可能在等待 begin_op 的进程
    wakeup(&log);
  }
  release(&log.lock);

  if(do_commit){
    commit();
    acquire(&log.lock);
    log.committing = 0;
    wakeup(&log);
    release(&log.lock);
  }
}

// 将修改过的块记录到日志中
static void write_log(void) {
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *to = bread(log.dev, log.start + tail + 1); // 日志区
    struct buf *from = bread(log.dev, log.lh.block[tail]); // 缓存区
    memmove(to->data, from->data, BSIZE);
    bwrite(to);
    brelse(from);
    brelse(to);
  }
}

static void commit() {
  if (log.lh.n > 0) {
    write_log();     // 1. 写日志数据块到磁盘日志区
    write_head();    // 2. 写日志头 (Commit point!)
    install_trans(0);// 3. 将数据写入实际位置
    log.lh.n = 0;
    write_head();    // 4. 清空日志头
  }
}

// 替代 bwrite，在此处被调用。
// 它不直接写入磁盘，而是将块标记为“脏”并加入日志列表。
void log_write(struct buf *b) {
  int i;

  if (log.lh.n >= log.size || log.lh.n >= 30 - 1)
    panic("too big a transaction");
  if (log.outstanding < 1)
    panic("log_write outside of trans");

  acquire(&log.lock);
  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // 已经记录过了
      break;
  }
  log.lh.block[i] = b->blockno;
  if (i == log.lh.n) {  // 新增记录
    bpin(b); // 钉住缓存，防止被驱逐
    log.lh.n++;
  }
  release(&log.lock);
}