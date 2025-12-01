# 实验八：内核日志系统 (Lab 8 Kernel Logging)

## 实验目标

在完成 Lab1-Lab7 的内核基础能力之后，本实验聚焦“可观测性”。我们实现了一个结构化的内核日志框架，满足以下需求：

1. **分级日志**：支持 `TRACE/DEBUG/INFO/WARN/ERROR/FATAL` 六档可配置的缓冲级别与控制台级别。
2. **环形缓冲区**：提供 256 条定长记录，包含时间戳、CPU、PID、组件和格式化消息，既能高吞吐写入也能追溯最近事件。
3. **输出策略**：独立控制“写入缓冲”与“立即刷到控制台”，可在测试期间抑制噪声，在调试时放大关键告警。
4. **自检工具**：在 `kernel/test.c` 中新增 Lab8 测试套件，验证统计计数、过滤行为以及 dump/summary 接口，确保功能可回归。

## 关键模块

### `kernel/klog.c`
- 维护 `klog_state`：自旋锁保护的环形缓冲、总计数、溢出计数、控制台状态。
- 实现 `klog_write()`：格式化消息后按级别写入缓冲，并可选同步到控制台。
- 提供 `klog_dump_recent()`、`klog_summary()`、`klog_get_stats()` 等调试 API，方便现场分析。

### `kernel/klog.h`
- 定义枚举级别、统计结构体以及 `KLOG_*` 宏，方便在任意子系统内一行写日志。

### 典型落点
- `kmain()`、`main_task()`、`proc.c` 等核心路径内加入 `KLOG_INFO/WARN/FATAL`，展示如何在调度、进程生命周期、启动流程中埋点。

### 测试 (`kernel/test.c`)
- **Lab8 Test 1 – Ring Buffer Counters**：生成三条日志，断言总数与存储量同步增长。
- **Lab8 Test 2 – Level Filtering**：将缓冲级别提升至 ERROR，确认 INFO/WARN 被过滤。
- **Lab8 Test 3 – Dump & Summary**：批量写入后调用 dump/summary，验证可视化接口。

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
- 提供 `sys_klog_*` 系统调用，把缓冲内容暴露给用户态调试工具。
- 为日志增加时间范围过滤、二进制导出，便于离线分析。

> 阅读顺序推荐：`kernel/klog.[ch]` → `kernel/main.c` → `kernel/proc.c` → `kernel/test.c`