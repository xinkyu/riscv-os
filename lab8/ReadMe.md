# 实验八：内核日志系统 (Lab 8 Kernel Logging)

## 实验目标

在完成 Lab1-Lab7 的内核基础能力之后，本实验聚焦“可观测性”。我们实现了一个结构化的内核日志框架，满足以下需求：

1. **分级日志**：支持 `TRACE/DEBUG/INFO/WARN/ERROR/FATAL` 六档可配置的缓冲级别与控制台级别。
2. **环形缓冲区**：提供 256 条定长记录，包含时间戳、CPU、PID、组件和格式化消息，既能高吞吐写入也能追溯最近事件。
3. **输出策略**：独立控制“写入缓冲”与“立即刷到控制台”，可在测试期间抑制噪声，在调试时放大关键告警。
4. **自检工具**：在 `kernel/test.c` 中新增 Lab8 测试套件，验证统计计数、过滤行为以及 dump/summary 接口，确保功能可回归。
5. **用户态可观察性**：新增 `sys_klog` 系统调用与 `klog_read()` 导出通道，配套用户态循环读取程序，实现 “dmesg -c” 式的流式消费能力。

## 关键模块

### `kernel/klog.c`
- 维护 `klog_state`：自旋锁保护的环形缓冲、总计数、溢出计数、控制台状态。
- 实现 `klog_write()`：格式化消息后按级别写入缓冲，并可选同步到控制台。
- 提供 `klog_dump_recent()`、`klog_summary()`、`klog_get_stats()` 等调试 API，方便现场分析。

### `kernel/klog.h`
- 定义枚举级别、统计结构体以及 `KLOG_*` 宏，方便在任意子系统内一行写日志。

### 用户态导出路径 (`klog_read` + `sys_klog`)
- `klog_read()`：在内核层将最旧的记录格式化为文本行，消费环形缓冲并写入用户提供的缓冲区。
- `sys_klog()`：在 `sysproc.c` 中暴露给用户态，通过 `SYS_klog` 号由 `kernel/syscall.c` 分发。
- `test.c` 中提供 `stub_klog()`，任何用户程序只需准备一个缓冲区并循环调用即可实现日志抓取。

### 典型落点
- `kmain()`、`main_task()`、`proc.c` 等核心路径内加入 `KLOG_INFO/WARN/FATAL`，展示如何在调度、进程生命周期、启动流程中埋点。

### 测试 (`kernel/test.c`)
- **Lab8 Test 1 – Ring Buffer Counters**：生成三条日志，断言总数与存储量同步增长。
- **Lab8 Test 2 – Level Filtering**：将缓冲级别提升至 ERROR，确认 INFO/WARN 被过滤。
- **Lab8 Test 3 – Dump & Summary**：批量写入后调用 dump/summary，验证可视化接口。
- **Lab8 Test 4 – Ring Buffer Overflow**：构造 300+ 条记录验证覆盖计数与回绕逻辑。
- **Lab8 Test 5 – Formatting & Truncation**：使用长字符串和多种占位符检查 `kvsnprintf` 的健壮性。
- **Lab8 Test 6 – sys_klog Streaming**：用户态循环调用 `sys_klog`，以 128 字节块持续打印环形缓冲内容，模拟轻量级 `dmesg` 工具。

## 用户态日志监听程序

`kernel/test.c` 中的 `test_klog_syscall_stream()` 展示了最小可用的用户工具：

1. 通过 `KLOG_INFO` 预先写入多条记录，确保环形缓冲有内容可读。
2. 使用 `stub_klog(buf, sizeof(buf))` 反复抓取，直到系统调用返回 0，期间每次都把读到的文本块用 `printf` 打印出来。
3. 可以直接复用这段逻辑到自定义进程，实现后台日志采集、文件落盘或过滤分析。

运行 `make run` 时，Lab8 自检会自动触发该监听程序；若想独立验证，可在 `test.c` 里单独调用 `test_klog_syscall_stream()`。

## 使用方法

```bash
# 清理并构建 Lab8 内核
cd lab8
make clean && make

# 运行内核并观察测试输出
make run
```

运行过程中，Lab6/Lab7 测试先执行，随后出现 “Lab8 Kernel Logging Tests” 字样，即代表新日志系统的自检开始。看到 `===== Lab8 Kernel Logging Tests Completed =====` 以及最终的 `===== All Labs Complete =====` 表示全部通过。

## 进一步扩展建议
- 将 `klog_write` 嵌入更多子系统（虚拟内存、IPC、VFS），形成统一观测面。
- 为用户态提供简单的过滤/grep 能力，例如按组件或级别筛选输出。
- 将日志落盘到持久化块设备，结合 `virtio` 驱动做离线分析，或实现基于时间戳的范围查询。

> 阅读顺序推荐：`kernel/klog.[ch]` → `kernel/main.c` → `kernel/proc.c` → `kernel/test.c`