// kernel/fs.h
#ifndef __FS_H__
#define __FS_H__

#include "buf.h"

// 磁盘上的超级块 (Superblock)
//
struct superblock {
  uint magic;       // 必须是 FSMAGIC
  uint size;        // 文件系统总块数
  uint nblocks;     // 数据块数量
  uint ninodes;     // inode 数量
  uint nlog;        // 日志块数量
  uint logstart;    // 日志起始块号
  uint inodestart;  // inode 区起始块号
  uint bmapstart;   // bitmap 区起始块号
};

#define FSMAGIC 0x10203040

// 磁盘上的 Inode 结构 (dinode)
//
#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

struct dinode {
  short type;           // 文件类型 (T_DIR, T_FILE, T_DEVICE)
  short major;          // 主设备号 (仅 T_DEVICE)
  short minor;          // 次设备号 (仅 T_DEVICE)
  short nlink;          // 硬链接数量
  uint size;            // 文件大小 (字节)
  uint addrs[NDIRECT+1]; // 数据块地址 (12个直接块 + 1个间接块)
};

// Inode 类型
#define T_DIR     1  // 目录
#define T_FILE    2  // 普通文件
#define T_DEVICE  3  // 设备

// 内存中的 Inode 结构
//
struct inode {
  uint dev;             // 设备号
  uint inum;            // Inode 编号
  int ref;              // 内存引用计数
  struct sleeplock lock; // 保护 Inode 内容
  int valid;            // Inode 是否已从磁盘加载

  // dinode 中的内容拷贝
  short type;
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1];
};

// 目录项 (Directory entry)
//
#define DIRSIZ 14
struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

#endif // __FS_H__