#ifndef __PARAM_H__
#define __PARAM_H__

#define NPROC        64  // 最大进程数
#define NCPU          8  // 最大 CPU 数
#define NOFILE       16  // 每个进程打开的最大文件数
#define NFILE       100  // 系统总共打开的最大文件数
#define NINODE       50  // 内存中缓存的最大 Inode 数
#define NDEV         10  // 最大设备数
#define ROOTDEV       1  // 根文件系统设备号
#define MAXARG       32  // exec 的最大参数个数
#define MAXOPBLOCKS  10  // 每个文件系统操作涉及的最大块数
#define LOGSIZE      (MAXOPBLOCKS*3)  // 日志的最大块数
#define NBUF         (MAXOPBLOCKS*3)  // 块缓存大小
#define FSSIZE       1000  // 文件系统大小 (块)
#define MAXPATH      128   // [新增] 最大路径长度

#endif