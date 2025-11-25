#include "defs.h"
#include "param.h"
#include "fs.h"
#include "buf.h"

struct logheader {
    int n;
    int block[LOGSIZE];
};

struct log {
    struct spinlock lock;
    int start;
    int size;
    int outstanding;
    int committing;
    int dev;
    struct logheader lh;
};

static struct log log;

static void read_head(void);
static void write_head(void);
static void write_log(void);
static void install_trans(int recovering);

static void install_trans(int recovering) {
    for (int tail = 0; tail < log.lh.n; tail++) {
        struct buf *lbuf = bread(log.dev, log.start + tail + 1);
        struct buf *dbuf = bread(log.dev, log.lh.block[tail]);
        memmove(dbuf->data, lbuf->data, BSIZE);
        bwrite(dbuf);
        bunpin(dbuf);
        brelse(lbuf);
        brelse(dbuf);
    }
    if (!recovering) {
        log.lh.n = 0;
        struct buf *buf = bread(log.dev, log.start);
        struct logheader *hb = (struct logheader*)(buf->data);
        *hb = log.lh;
        bwrite(buf);
        brelse(buf);
    }
}

static void read_head(void) {
    struct buf *buf = bread(log.dev, log.start);
    struct logheader *hb = (struct logheader*)(buf->data);
    log.lh = *hb;
    brelse(buf);
}

static void write_head(void) {
    struct buf *buf = bread(log.dev, log.start);
    struct logheader *hb = (struct logheader*)(buf->data);
    *hb = log.lh;
    bwrite(buf);
    brelse(buf);
}

void initlog(int dev, struct superblock *sb) {
    if (sizeof(struct logheader) >= BSIZE) {
        panic("initlog: too big");
    }
    spinlock_init(&log.lock, "log");
    log.start = sb->logstart;
    log.size = sb->nlog;
    log.dev = dev;
    read_head();
    install_trans(1);
    log.lh.n = 0;
    write_head();
}

void begin_op(void) {
    acquire(&log.lock);
    while (1) {
        if (log.committing) {
            sleep(&log, &log.lock);
        } else if (log.lh.n + (log.outstanding + 1) * MAXOPBLOCKS > LOGSIZE) {
            sleep(&log, &log.lock);
        } else {
            log.outstanding += 1;
            release(&log.lock);
            break;
        }
    }
}

void end_op(void) {
    int do_commit = 0;

    acquire(&log.lock);
    log.outstanding--;
    if (log.committing) {
        panic("log.committing");
    }
    if (log.outstanding == 0) {
        do_commit = 1;
        log.committing = 1;
    } else {
        wakeup(&log);
    }
    release(&log.lock);

    if (do_commit) {
        if (log.lh.n > 0) {
            write_log();
            install_trans(0);
            log.lh.n = 0;
            write_head();
        }
        acquire(&log.lock);
        log.committing = 0;
        wakeup(&log);
        release(&log.lock);
    }
}

static void write_log(void) {
    for (int tail = 0; tail < log.lh.n; tail++) {
        struct buf *to = bread(log.dev, log.start + tail + 1);
        struct buf *from = bread(log.dev, log.lh.block[tail]);
        memmove(to->data, from->data, BSIZE);
        bwrite(to);
        brelse(from);
        brelse(to);
    }
}

void log_write(struct buf *b) {
    if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1) {
        panic("log_write: too big");
    }
    if (log.outstanding < 1) {
        panic("log_write outside trans");
    }

    acquire(&log.lock);
    int i;
    for (i = 0; i < log.lh.n; i++) {
        if (log.lh.block[i] == b->blockno) {
            break;
        }
    }
    log.lh.block[i] = b->blockno;
    if (i == log.lh.n) {
        log.lh.n++;
        bpin(b);
    }
    release(&log.lock);
}
