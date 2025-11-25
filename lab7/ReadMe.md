# 实验七：日志文件系统 (Lab 7 Filesystem)

## 实验目标

在此前完成系统调用、进程管理与设备驱动之后，本实验聚焦于“让数据可靠着陆”。核心目标包括：

1. 构建基于位图的块分配器与 inode 树，支持普通文件与目录操作。
2. 搭建 `bio` 缓冲区缓存、`sleeplock` 保护以及写前日志（Write-Ahead Logging）。
3. 通过 `sys_open/close/read/write/link/unlink/mkdir/mknod/chdir` 等系统调用形成最小可用的 POSIX 风格接口。
4. 设计内核自测用例，覆盖完整生命周期：建文件 → 并发访问 → 崩溃恢复 → 性能压测。

##  核心模块

### 1. 块与 inode 管理 (`kernel/fs.c`)
- **位图扫描 (`balloc/bfree`)**：遍历 `BBLOCK`，仅在数据区起始块之后进行分配；重复释放立即 `panic`，防御双重释放。
- **inode 缓存 (`icache`)**：`sleeplock` 全程保护，`iget/ilock/iupdate/itrunc` 管理生命周期与落盘。
- **`bmap`**：提供 12 个直接块 + 1 个间接块，最大文件大小 `MAXFILE * BSIZE`。

### 2. 写前日志 (Write-Ahead Logging, `kernel/log.c`)
- `begin_op/end_op` 控制事务数量并阻止超过 `LOGSIZE`。
- `log_write` 仅在块第一次进入日志时 `bpin`，避免无限增加 `refcnt`。
- `install_trans` 在回放日志后立即 `bunpin`，确保缓冲区最终可回收。

### 3. 系统调用到文件系统栈 (`kernel/sysfile.c`, `kernel/file.c`)
- 实现 `sys_open/close/read/write/link/unlink/mkdir/mknod/chdir/fstat` 等接口，贯通 VFS → inode → 块缓存。
- `filewrite` 使用“分块 + 事务”策略，一次写入不会撑爆日志，保证原子性与性能的平衡。

### 4. 自定义测试框架 (`kernel/test.c`)
- **FS Test 1 – Integrity**：创建 → 写入 → 读取 → unlink，验证基础读写与目录操作。
- **FS Test 2 – Concurrent Access**：多个进程竞争同一目录，检验日志串行化与 `bio` 的正确性。
- **FS Test 3 – Crash Recovery**：手动调用 `initlog()` 模拟重启，确保日志重放能恢复一致状态。
- **FS Test 4 – Performance**：生成 200 个小文件与 4MB 大文件，输出耗时、缓存命中、磁盘 IO 统计。

## 运行与验证

```bash
# 清理旧构建
make clean

# 编译并运行所有内核内测试（Lab6 + Lab7）
make run
```

QEMU 控制台会打印每一阶段的详细日志；出现 `===== All Labs Complete =====` 即代表所有实验通过。

##  实验成果摘要
- 实现 xv6 风格的日志文件系统，提供稳定的 POSIX 基础操作集。
- 修复位图分配与日志 pin/unpin 的一致性问题，长时间压力测试不再 panic。
- 内置测试覆盖功能、并发、崩溃恢复与性能场景，便于回归与扩展。

> 可重点阅读 `kernel/fs.c`, `kernel/log.c`, `kernel/bio.c`, `kernel/sysfile.c` 四个文件，构成了 Lab7 的核心。