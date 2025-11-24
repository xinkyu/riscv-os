#include "defs.h"
#include "riscv.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h" // [新增] 显式引用锁定义
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"     // [新增] 必须包含，这里定义了 struct inode
#include "fcntl.h"
#include "buf.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

// 有 2 个超级块：磁盘上的和内存中的
struct superblock sb; 

// 读取超级块
static void read_superblock(int dev, struct superblock *sb) {
  struct buf *bp;
  bp = bread(dev, 1);
  memmove(sb, bp->data, sizeof(*sb));
  brelse(bp);
}

// 初始化文件系统
void fsinit(int dev) {
  read_superblock(dev, &sb);
  if(sb.magic != FSMAGIC)
    panic("invalid file system");
  initlog(dev, &sb);
}

// 块清零
static void bzero(int dev, int bno) {
  struct buf *bp;
  bp = bread(dev, bno);
  memset(bp->data, 0, BSIZE);
  log_write(bp);
  brelse(bp);
}

// 分配一个新的磁盘块
static uint balloc(uint dev) {
  int b, bi, m;
  struct buf *bp;

  bp = 0;
  for(b = 0; b < sb.size; b += BPB){
    bp = bread(dev, IBLOCK(0, sb) + sb.ninodes/IPB + 1 + b/BPB); // 简单的位图位置计算
    for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
      m = 1 << (bi % 8);
      if((bp->data[bi/8] & m) == 0){  // 找到空闲位
        bp->data[bi/8] |= m;  // 标记为已用
        log_write(bp);
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

// 释放磁盘块
static void bfree(int dev, uint b) {
  struct buf *bp;
  int bi, m;

  bp = bread(dev, IBLOCK(0, sb) + sb.ninodes/IPB + 1 + b/BPB); // 位图位置
  bi = b % BPB;
  m = 1 << (bi % 8);
  if((bp->data[bi/8] & m) == 0)
    panic("freeing free block");
  bp->data[bi/8] &= ~m;
  log_write(bp);
  brelse(bp);
}

// ================== Inode 层 ==================

struct {
  struct spinlock lock;
  struct inode inode[50]; // Inode 缓存大小
} icache;

void iinit() {
  int i;
  initlock(&icache.lock, "icache");
  for(i = 0; i < 50; i++) {
    initsleeplock(&icache.inode[i].lock, "inode"); // [修改] 使用 initsleeplock
  }
}

// 分配新的 Inode
struct inode* ialloc(uint dev, short type) {
  int inum;
  struct buf *bp;
  struct dinode *dip;

  for(inum = 1; inum < sb.ninodes; inum++){
    bp = bread(dev, IBLOCK(inum, sb));
    dip = (struct dinode*)bp->data + inum%IPB;
    if(dip->type == 0){  // 找到空闲 Inode
      memset(dip, 0, sizeof(*dip));
      dip->type = type;
      log_write(bp);   // 标记 Inode 块已修改
      brelse(bp);
      return iget(dev, inum);
    }
    brelse(bp);
  }
  panic("ialloc: no inodes");
  return 0;
}

// 将内存 Inode 更新到磁盘
void iupdate(struct inode *ip) {
  struct buf *bp;
  struct dinode *dip;

  bp = bread(ip->dev, IBLOCK(ip->inum, sb));
  dip = (struct dinode*)bp->data + ip->inum%IPB;
  dip->type = ip->type;
  dip->major = ip->major;
  dip->minor = ip->minor;
  dip->nlink = ip->nlink;
  dip->size = ip->size;
  memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
  log_write(bp);
  brelse(bp);
}

// 获取 Inode (增加引用计数，如果不在缓存则加载)
struct inode* iget(uint dev, uint inum) {
  struct inode *ip, *empty;

  acquire(&icache.lock);

  // 1. 检查是否在缓存中
  for(ip = &icache.inode[0]; ip < &icache.inode[50]; ip++){
    if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
      ip->ref++;
      release(&icache.lock);
      return ip;
    }
  }

  // 2. 分配新的缓存项
  empty = 0;
  for(ip = &icache.inode[0]; ip < &icache.inode[50]; ip++){
    if(ip->ref == 0){
      empty = ip;
      break;
    }
  }

  if(empty == 0)
    panic("iget: no inodes");

  ip = empty;
  ip->dev = dev;
  ip->inum = inum;
  ip->ref = 1;
  ip->valid = 0;
  release(&icache.lock);

  return ip;
}

// 锁定 Inode 并从磁盘读取内容（如果尚未读取）
void ilock(struct inode *ip) {
  struct buf *bp;
  struct dinode *dip;

  if(ip == 0 || ip->ref < 1)
    panic("ilock");

  acquiresleep(&ip->lock);

  if(ip->valid == 0){
    bp = bread(ip->dev, IBLOCK(ip->inum, sb));
    dip = (struct dinode*)bp->data + ip->inum%IPB;
    ip->type = dip->type;
    ip->major = dip->major;
    ip->minor = dip->minor;
    ip->nlink = dip->nlink;
    ip->size = dip->size;
    memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
    brelse(bp);
    ip->valid = 1;
    if(ip->type == 0)
      panic("ilock: no type");
  }
}

// 解锁 Inode
void iunlock(struct inode *ip) {
  if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
    panic("iunlock");
  releasesleep(&ip->lock);
}

// 减少引用计数
void iput(struct inode *ip) {
  acquire(&icache.lock);

  if(ip->ref == 1 && ip->valid && ip->nlink == 0){
    // 如果没有链接且没有引用，则释放 Inode 及其占用的数据块
    release(&icache.lock);
    
    // 我们需要锁来调用 itrunc
    acquiresleep(&ip->lock);
    itrunc(ip);
    ip->type = 0;
    iupdate(ip); // 更新到磁盘：type=0 表示释放
    ip->valid = 0;
    releasesleep(&ip->lock);

    acquire(&icache.lock);
  }

  ip->ref--;
  release(&icache.lock);
}

void iunlockput(struct inode *ip) {
  iunlock(ip);
  iput(ip);
}

// Inode 块映射: 返回文件逻辑块 bn 对应的磁盘物理块号
// 如果 alloc != 0，则在需要时分配块
static uint bmap(struct inode *ip, uint bn) {
  uint addr, *a;
  struct buf *bp;

  if(bn < NDIRECT){
    if((addr = ip->addrs[bn]) == 0)
      ip->addrs[bn] = addr = balloc(ip->dev);
    return addr;
  }
  bn -= NDIRECT;

  if(bn < NINDIRECT){
    // 加载间接块
    if((addr = ip->addrs[NDIRECT]) == 0)
      ip->addrs[NDIRECT] = addr = balloc(ip->dev);
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[bn]) == 0){
      a[bn] = addr = balloc(ip->dev);
      log_write(bp); // 记录间接块的修改
    }
    brelse(bp);
    return addr;
  }

  panic("bmap: out of range");
  return 0;
}

// 截断 Inode (释放所有数据块)
void itrunc(struct inode *ip) {
  int i, j;
  struct buf *bp;
  uint *a;

  for(i = 0; i < NDIRECT; i++){
    if(ip->addrs[i]){
      bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }

  if(ip->addrs[NDIRECT]){
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    a = (uint*)bp->data;
    for(j = 0; j < NINDIRECT; j++){
      if(a[j])
        bfree(ip->dev, a[j]);
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }

  ip->size = 0;
  iupdate(ip);
}

// 读取 Inode 数据
int readi(struct inode *ip, int user_dst, uint64 dst, uint off, uint n) {
  uint tot, m;
  struct buf *bp;

  if(off > ip->size || off + n < off)
    return 0;
  if(off + n > ip->size)
    n = ip->size - off;

  for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
    if(user_dst){
      if(copyout(myproc()->pagetable, dst, (char*)bp->data + off%BSIZE, m) == -1) {
        brelse(bp);
        return -1;
      }
    } else {
      memmove((void*)dst, (char*)bp->data + off%BSIZE, m);
    }
    brelse(bp);
  }
  return n;
}

// 写入 Inode 数据
int writei(struct inode *ip, int user_src, uint64 src, uint off, uint n) {
  uint tot, m;
  struct buf *bp;

  if(off > ip->size || off + n < off)
    return -1;
  if(off + n > MAXFILE*BSIZE)
    return -1;

  for(tot=0; tot<n; tot+=m, off+=m, src+=m){
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
    if(user_src){
      if(copyin(myproc()->pagetable, (char*)bp->data + off%BSIZE, src, m) == -1) {
        brelse(bp);
        return -1;
      }
    } else {
      memmove((char*)bp->data + off%BSIZE, (void*)src, m);
    }
    log_write(bp);
    brelse(bp);
  }

  if(n > 0){
    if(off > ip->size)
      ip->size = off;
    iupdate(ip);
  }
  return n;
}

// 复制 Inode
struct inode* idup(struct inode *ip) {
  acquire(&icache.lock);
  ip->ref++;
  release(&icache.lock);
  return ip;
}

// 填充 stat 结构
void stati(struct inode *ip, struct stat *st) {
  st->dev = ip->dev;
  st->ino = ip->inum;
  st->type = ip->type;
  st->nlink = ip->nlink;
  st->size = ip->size;
}

// ================== 目录层 ==================

int namecmp(const char *s, const char *t) {
  return strncmp(s, t, DIRSIZ);
}

// 在目录 dp 中查找名字为 name 的条目
struct inode* dirlookup(struct inode *dp, char *name, uint *poff) {
  uint off, inum;
  struct dirent de;

  if(dp->type != T_DIR)
    panic("dirlookup not DIR");

  for(off = 0; off < dp->size; off += sizeof(de)){
    if(readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
      panic("dirlookup read");
    if(de.inum == 0)
      continue;
    if(namecmp(name, de.name) == 0){
      // 找到了
      if(poff)
        *poff = off;
      return iget(dp->dev, de.inum);
    }
  }
  return 0;
}

// 在目录 dp 中增加一个新条目 (name, inum)
int dirlink(struct inode *dp, char *name, uint inum) {
  int off;
  struct dirent de;
  struct inode *ip;

  // 检查名字是否存在
  if((ip = dirlookup(dp, name, 0)) != 0){
    iput(ip);
    return -1;
  }

  // 找空槽位
  for(off = 0; off < dp->size; off += sizeof(de)){
    if(readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
      panic("dirlink read");
    if(de.inum == 0)
      break;
  }

  strncpy(de.name, name, DIRSIZ);
  de.inum = inum;
  if(writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
    panic("dirlink");
  
  return 0;
}

// ================== 路径层 ==================

// 路径解析辅助函数：获取下一个路径元素
static char* skipelem(char *path, char *name) {
  char *s;
  int len;

  while(*path == '/')
    path++;
  if(*path == 0)
    return 0;
  s = path;
  while(*path != '/' && *path != 0)
    path++;
  len = path - s;
  if(len >= DIRSIZ)
    memmove(name, s, DIRSIZ);
  else {
    memmove(name, s, len);
    name[len] = 0;
  }
  while(*path == '/')
    path++;
  return path;
}

static struct inode* namex(char *path, int nameiparent, char *name) {
  struct inode *ip, *next;

  if(*path == '/')
    ip = iget(ROOTDEV, ROOTINO);
  else
    ip = idup(myproc()->cwd);

  while((path = skipelem(path, name)) != 0){
    ilock(ip);
    if(ip->type != T_DIR){
      iunlockput(ip);
      return 0;
    }
    if(nameiparent && *path == '\0'){
      // 停止在父目录
      iunlock(ip);
      return ip;
    }
    if((next = dirlookup(ip, name, 0)) == 0){
      iunlockput(ip);
      return 0;
    }
    iunlockput(ip);
    ip = next;
  }
  if(nameiparent){
    iput(ip);
    return 0;
  }
  return ip;
}

struct inode* namei(char *path) {
  char name[DIRSIZ];
  return namex(path, 0, name);
}

struct inode* nameiparent(char *path, char *name) {
  return namex(path, 1, name);
}