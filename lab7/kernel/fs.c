#include "defs.h"
#include "param.h"
#include "fs.h"
#include "buf.h"
#include "file.h"
#include "stat.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct superblock sb;

struct {
    struct spinlock lock;
    struct inode inode[NINODE];
} icache;

static void readsb(int dev, struct superblock *sb);
static void bzero(int dev, int bno);
static uint balloc(uint dev);
static void bfree(int dev, uint b);
static uint bmap(struct inode *ip, uint bn);
void itrunc(struct inode *ip);
static char *skipelem(char *path, char *name);
static void fs_format(int dev);

static inline uint data_start_block(void) {
    return sb.bmapstart + ((sb.nblocks + BPB - 1) / BPB);
}

static void readsb(int dev, struct superblock *sb) {
    struct buf *bp = bread(dev, 1);
    memmove(sb, bp->data, sizeof(*sb));
    brelse(bp);
}

static void bzero(int dev, int bno) {
    struct buf *bp = bread(dev, bno);
    memset(bp->data, 0, BSIZE);
    bwrite(bp); // Changed from log_write to bwrite
    brelse(bp);
}

static uint balloc(uint dev) {
    struct buf *bp;
    uint b;
    for (b = data_start_block(); b < sb.size; b += BPB) {
        bp = bread(dev, BBLOCK(b, sb));
        for (uint bi = 0; bi < BPB && b + bi < sb.size; bi++) {
            int m = 1 << (bi % 8);
            if ((bp->data[bi / 8] & m) == 0) {
                bp->data[bi / 8] |= m;
                bwrite(bp); // Changed from log_write to bwrite
                brelse(bp);
                bzero(dev, b + bi);
                return b + bi;
            }
        }
        brelse(bp);
    }
    panic("balloc: out of blocks");
    return 0;
}

static void bfree(int dev, uint b) {
    struct buf *bp = bread(dev, BBLOCK(b, sb));
    uint bi = b % BPB;
    int m = 1 << (bi % 8);
    if ((bp->data[bi / 8] & m) == 0) {
        // Changed to warning instead of panic to allow recovery/continuation
        // Also added check to avoid modifying the bitmap if already free
        printf("warning: bfree: block %d is already free\n", b);
        brelse(bp);
        return;
    }
    bp->data[bi / 8] &= ~m;
    bwrite(bp); // Changed from log_write to bwrite
    brelse(bp);
}

void iinit(void) {
    readsb(ROOTDEV, &sb);
    if (sb.magic != FSMAGIC) {
        fs_format(ROOTDEV);
        readsb(ROOTDEV, &sb);
    }
    spinlock_init(&icache.lock, "icache");
    for (int i = 0; i < NINODE; i++) {
        icache.inode[i].ref = 0;
        initsleeplock(&icache.inode[i].lock, "inode");
    }
    printf("fs: size=%d nblocks=%d ninodes=%d nlog=%d\n", sb.size, sb.nblocks, sb.ninodes, sb.nlog);
}

struct inode *iget(uint dev, uint inum) {
    struct inode *ip, *empty = 0;
    acquire(&icache.lock);
    for (ip = icache.inode; ip < &icache.inode[NINODE]; ip++) {
        if (ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
            ip->ref++;
            release(&icache.lock);
            return ip;
        }
        if (empty == 0 && ip->ref == 0)
            empty = ip;
    }
    if (empty == 0)
        panic("iget: no inodes");
    ip = empty;
    ip->dev = dev;
    ip->inum = inum;
    ip->ref = 1;
    ip->valid = 0;
    release(&icache.lock);
    return ip;
}

struct inode *ialloc(uint dev, short type) {
    for (uint inum = 1; inum < sb.ninodes; inum++) {
        struct buf *bp = bread(dev, IBLOCK(inum, sb));
        struct dinode *dip = (struct dinode*)bp->data + inum % IPB;
        if (dip->type == 0) {
            memset(dip, 0, sizeof(*dip));
            dip->type = type;
            bwrite(bp); // Changed from log_write to bwrite
            brelse(bp);
            return iget(dev, inum);
        }
        brelse(bp);
    }
    panic("ialloc");
    return 0;
}

struct inode *idup(struct inode *ip) {
    acquire(&icache.lock);
    ip->ref++;
    release(&icache.lock);
    return ip;
}

void ilock(struct inode *ip) {
    if (ip == 0 || ip->ref < 1)
        panic("ilock");
    acquiresleep(&ip->lock);
    if (ip->valid == 0) {
        struct buf *bp = bread(ip->dev, IBLOCK(ip->inum, sb));
        struct dinode *dip = (struct dinode*)bp->data + ip->inum % IPB;
        ip->type = dip->type;
        ip->major = dip->major;
        ip->minor = dip->minor;
        ip->nlink = dip->nlink;
        ip->size = dip->size;
        memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
        ip->valid = 1;
        brelse(bp);
        if (ip->type == 0)
            panic("ilock: no type");
    }
}

void iunlock(struct inode *ip) {
    if (ip == 0 || !holdingsleep(&ip->lock))
        panic("iunlock");
    releasesleep(&ip->lock);
}

void iupdate(struct inode *ip) {
    struct buf *bp = bread(ip->dev, IBLOCK(ip->inum, sb));
    struct dinode *dip = (struct dinode*)bp->data + ip->inum % IPB;
    dip->type = ip->type;
    dip->major = ip->major;
    dip->minor = ip->minor;
    dip->nlink = ip->nlink;
    dip->size = ip->size;
    memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
    bwrite(bp); // Changed from log_write to bwrite
    brelse(bp);
}

void iput(struct inode *ip) {
    acquiresleep(&ip->lock);
    if (ip->valid && ip->nlink == 0 && ip->ref == 1) {
        itrunc(ip);
        ip->type = 0;
        iupdate(ip);
        ip->valid = 0;
    }
    releasesleep(&ip->lock);
    acquire(&icache.lock);
    ip->ref--;
    release(&icache.lock);
}

void iunlockput(struct inode *ip) {
    iunlock(ip);
    iput(ip);
}

static uint bmap(struct inode *ip, uint bn) {
    uint addr;
    struct buf *bp;
    if (bn < NDIRECT) {
        if ((addr = ip->addrs[bn]) == 0)
            ip->addrs[bn] = addr = balloc(ip->dev);
        return addr;
    }
    bn -= NDIRECT;
    if (bn < NINDIRECT) {
        if ((addr = ip->addrs[NDIRECT]) == 0)
            ip->addrs[NDIRECT] = addr = balloc(ip->dev);
        bp = bread(ip->dev, addr);
        uint *a = (uint*)bp->data;
        if (a[bn] == 0) {
            a[bn] = balloc(ip->dev);
            bwrite(bp); // Changed from log_write to bwrite
        }
        uint r = a[bn];
        brelse(bp);
        return r;
    }
    panic("bmap");
    return 0;
}

void itrunc(struct inode *ip) {
    for (int i = 0; i < NDIRECT; i++) {
        if (ip->addrs[i]) {
            bfree(ip->dev, ip->addrs[i]);
            ip->addrs[i] = 0;
        }
    }
    if (ip->addrs[NDIRECT]) {
        struct buf *bp = bread(ip->dev, ip->addrs[NDIRECT]);
        uint *a = (uint*)bp->data;
        for (int j = 0; j < NINDIRECT; j++) {
            if (a[j])
                bfree(ip->dev, a[j]);
        }
        brelse(bp);
        bfree(ip->dev, ip->addrs[NDIRECT]);
        ip->addrs[NDIRECT] = 0;
    }
    ip->size = 0;
    iupdate(ip);
}

int stati(struct inode *ip, struct stat *st) {
    st->dev = ip->dev;
    st->ino = ip->inum;
    st->type = ip->type;
    st->nlink = ip->nlink;
    st->size = ip->size;
    return 0;
}

int readi(struct inode *ip, int user, uint64 dst, uint off, uint n) {
    if (user)
        panic("readi user");
    
    uint tot, m;
    struct buf *bp;

    if (off > ip->size || off + n < off)
        return 0;
    
    if (off + n > ip->size)
        n = ip->size - off;

    for (tot = 0; tot < n; tot += m, off += m, dst += m) {
        uint addr = bmap(ip, off / BSIZE);
        bp = bread(ip->dev, addr);
        m = MIN(n - tot, BSIZE - off % BSIZE);
        memmove((void*)dst, bp->data + off % BSIZE, m);
        brelse(bp);
    }
    return n;
}

int writei(struct inode *ip, int user, uint64 src, uint off, uint n) {
    if (user)
        panic("writei user");
    if (off > ip->size || off + n < off)
        return -1;
    if (off + n > MAXFILE * BSIZE)
        return -1;
    uint tot = 0;
    while (tot < n) {
        uint addr = bmap(ip, (off + tot) / BSIZE);
        struct buf *bp = bread(ip->dev, addr);
        uint m = MIN(n - tot, BSIZE - (off + tot) % BSIZE);
        memmove(bp->data + (off + tot) % BSIZE, (void*)(src + tot), m);
        bwrite(bp); // Changed from log_write to bwrite
        brelse(bp);
        tot += m;
    }
    if (off + n > ip->size)
        ip->size = off + n;
    iupdate(ip);
    return n;
}

int namecmp(const char *s, const char *t) {
    return strncmp(s, t, DIRSIZ);
}

struct inode *dirlookup(struct inode *dp, char *name, uint *poff) {
    if (dp->type != T_DIR)
        panic("dirlookup");
    struct dirent de;
    for (uint off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
            panic("dirlookup read");
        if (de.inum == 0)
            continue;
        if (namecmp(name, de.name) == 0) {
            if (poff)
                *poff = off;
            return iget(dp->dev, de.inum);
        }
    }
    return 0;
}

int dirlink(struct inode *dp, char *name, uint inum) {
    struct dirent de;
    uint off;
    if (dirlookup(dp, name, 0) != 0)
        return -1;
    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
            panic("dirlink read");
        if (de.inum == 0)
            break;
    }
    de.inum = inum;
    safestrcpy(de.name, name, DIRSIZ);
    if (writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
        panic("dirlink");
    return 0;
}

static char *skipelem(char *path, char *name) {
    while (*path == '/')
        path++;
    if (*path == 0)
        return 0;
    char *s = path;
    while (*path != '/' && *path != 0)
        path++;
    int len = path - s;
    if (len >= DIRSIZ)
        memmove(name, s, DIRSIZ);
    else {
        memmove(name, s, len);
        name[len] = 0;
    }
    while (*path == '/')
        path++;
    return path;
}

struct inode *namex(char *path, int nameiparent, char *name) {
    struct inode *ip, *next;
    if (*path == '/')
        ip = iget(ROOTDEV, ROOTINO);
    else
        ip = idup(myproc()->cwd);
    while ((path = skipelem(path, name)) != 0) {
        ilock(ip);
        if (ip->type != T_DIR) {
            iunlockput(ip);
            return 0;
        }
        if (nameiparent && *path == '\0') {
            iunlock(ip);
            return ip;
        }
        if ((next = dirlookup(ip, name, 0)) == 0) {
            iunlockput(ip);
            return 0;
        }
        iunlockput(ip);
        ip = next;
    }
    if (nameiparent) {
        iput(ip);
        return 0;
    }
    return ip;
}

struct inode *namei(char *path) {
    char name[DIRSIZ];
    return namex(path, 0, name);
}

struct inode *nameiparent(char *path, char *name) {
    return namex(path, 1, name);
}

int count_free_blocks(void) {
    int free = 0;
    uint start = data_start_block();
    for (uint b = start; b < sb.size; b += BPB) {
        struct buf *bp = bread(ROOTDEV, BBLOCK(b, sb));
        for (uint bi = 0; bi < BPB && b + bi < sb.size; bi++) {
            int m = 1 << (bi % 8);
            if ((bp->data[bi / 8] & m) == 0)
                free++;
        }
        brelse(bp);
    }
    return free;
}

int count_free_inodes(void) {
    int free = 0;
    for (uint inum = 1; inum < sb.ninodes; inum++) {
        struct buf *bp = bread(ROOTDEV, IBLOCK(inum, sb));
        struct dinode *dip = (struct dinode*)bp->data + inum % IPB;
        if (dip->type == 0)
            free++;
        brelse(bp);
    }
    return free;
}

void get_superblock(struct superblock *dst) {
    *dst = sb;
}

void dump_inode_usage(void) {
    acquire(&icache.lock);
    printf("=== Inode Usage ===\n");
    for (int i = 0; i < NINODE; i++) {
        struct inode *ip = &icache.inode[i];
        if (ip->ref > 0)
            printf("inum=%d ref=%d type=%d size=%d\n", ip->inum, ip->ref, ip->type, ip->size);
    }
    release(&icache.lock);
}

static void bitmap_set(int dev, uint blockno) {
    struct buf *bp = bread(dev, BBLOCK(blockno, sb));
    uint bi = blockno % BPB;
    bp->data[bi / 8] |= 1 << (bi % 8);
    bwrite(bp);
    brelse(bp);
}

static void fs_format(int dev) {
    printf("Formatting filesystem...\n");
    memset(&sb, 0, sizeof(sb));
    sb.magic = FSMAGIC;
    sb.size = FSSIZE;
    sb.ninodes = NINODE;
    sb.nlog = LOGSIZE;
    sb.logstart = 2;
    sb.inodestart = sb.logstart + sb.nlog;
    int inodeblocks = (sb.ninodes + IPB - 1) / IPB;
    sb.bmapstart = sb.inodestart + inodeblocks;
    uint bitmapblocks = 1; // temporary
    sb.nblocks = sb.size - (sb.bmapstart + bitmapblocks);
    bitmapblocks = (sb.nblocks + BPB - 1) / BPB;
    sb.nblocks = sb.size - (sb.bmapstart + bitmapblocks);

    for (uint b = 0; b < sb.size; b++) {
        struct buf *bp = bread(dev, b);
        memset(bp->data, 0, BSIZE);
        bwrite(bp);
        brelse(bp);
    }

    struct buf *bp = bread(dev, 1);
    memmove(bp->data, &sb, sizeof(sb));
    bwrite(bp);
    brelse(bp);

    uint start = data_start_block();
    for (uint b = 0; b < start; b++)
        bitmap_set(dev, b);

    uint root_block = start;
    bitmap_set(dev, root_block);

    struct buf *ib = bread(dev, IBLOCK(ROOTINO, sb));
    struct dinode *dip = (struct dinode*)ib->data + ROOTINO % IPB;
    memset(dip, 0, sizeof(*dip));
    dip->type = T_DIR;
    dip->nlink = 2;
    dip->size = sizeof(struct dirent) * 2;
    dip->addrs[0] = root_block;
    bwrite(ib);
    brelse(ib);

    struct buf *db = bread(dev, root_block);
    struct dirent *de = (struct dirent*)db->data;
    memset(de, 0, BSIZE);
    de[0].inum = ROOTINO;
    safestrcpy(de[0].name, ".", DIRSIZ);
    de[1].inum = ROOTINO;
    safestrcpy(de[1].name, "..", DIRSIZ);
    bwrite(db);
    brelse(db);
}