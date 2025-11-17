
# RISC-V 操作系统 Lab 5: 进程管理与调度

本项目是《从零构建操作系统》的第五个实验 [cite: 1063]。
本实验的目标是引入进程的抽象，实现一个支持多任务的内核 [cite: 1064]。

## 实验目标

* 理解进程的抽象，包括进程状态（`UNUSED`, `SLEEPING`, `RUNNABLE`, `RUNNING`, `ZOMBIE`）。
* 设计并实现进程控制块（`struct proc`）和进程表（`proc[NPROC]`）。
* 实现内核上下文切换的汇编代码（`swtch.S`）。
* 实现一个协作式的轮转（Round-Robin）调度器（`scheduler()`）。
* 实现进程的生命周期管理：`create_process`, `exit`, `wait` 。
* 实现基础的同步原语：`sleep` 和 `wakeup` 。

## 实现概览

### 1. 核心数据结构

* **`kernel/proc.h`**:
    * `struct context`: 定义了上下文切换时需要保存的 14 个寄存器（`ra`, `sp` 和 `s0-s11`）。
    * `enum procstate`: 定义了进程的 5 种状态。
    * `struct proc`: 核心的进程控制块，包含了进程的内核栈、上下文、状态、PID、父进程指针和锁。
* **`kernel/spinlock.h`**:
    * `struct spinlock`: 实现了自旋锁，用于在多核或中断环境中保护共享数据。

### 2. 进程生命周期

* **`kernel/proc.c`**:
    * `allocproc()`: 负责从进程表中查找一个 `UNUSED` 的 `proc` 结构体，为其分配内核栈，并初始化上下文，使其准备好在 `proc_entry` 处开始执行。
    * `create_process()`: `allocproc` 的封装，用于创建一个新的内核线程，并将其状态设置为 `RUNNABLE`。
    * `exit()`: 进程自我终止，将状态设置为 `ZOMBIE`，并唤醒其父进程。
    * `wait()`: 父进程调用，用于回收状态为 `ZOMBIE` 的子进程，并释放其资源。

### 3. 调度与同步

* **`kernel/swtch.S`**:
    * `swtch()`: 底层汇编函数，负责原子地保存当前上下文（`old`）并恢复新上下文（`new`）。
* **`kernel/proc.c`**:
    * `scheduler()`: 调度器主循环。它不断地遍历进程表，查找 `RUNNABLE` 状态的进程，并通过 `swtch` 切换到它。
    * `sched()`: 进程让出 CPU 的核心函数，它保存当前进程的上下文，并切换到调度器（`cpu->context`）的上下文。
    * `yield()`: `sched()` 的一个简单封装，允许进程主动让出 CPU。
    * `sleep(chan, lk)`: 关键的同步原语。它原子地释放传入的锁 `lk`，将进程状态设为 `SLEEPING`，然后调用 `sched()` 休眠。唤醒后，它会重新获取 `lk`。
    * `wakeup(chan)`: 遍历进程表，唤醒所有在同一 `chan` 上 `SLEEPING` 的进程，将它们的状态改回 `RUNNABLE`。

### 4. 中断处理

* **`kernel/trap.c`**:
    * 为防止内核栈溢出，已移除时钟中断中的抢占式 `yield()` 调用 。
    * 内核现在是一个**协作式多任务**内核。
    * 时钟中断现在的主要职责是递增 `tick_counter` 并 `wakeup` 任何在 `tick_counter` 上 `sleep` 的进程（例如 `kernel_sleep`）。

## 编译与运行

```bash
# 编译并运行
make run
````

## 测试结果

所有 Lab 3, Lab 4, 和 Lab 5 的测试均已通过：

```
===== All Lab4 Tests Passed! =====

===== Starting Lab5 Tests =====

=== Test 7: Process Creation ===
...
Created 63 processes (expected max 63)
...
Process creation test passed

=== Test 8: Scheduler ===
...
PID 66: cpu_intensive_task running...
PID 66: cpu_intensive_task finished.
PID 67: cpu_intensive_task running...
PID 67: cpu_intensive_task finished.
PID 68: cpu_intensive_task running...
PID 68: cpu_intensive_task finished.
Scheduler test completed (slept 1000 ticks)

=== Test 9: Synchronization (Producer-Consumer) ===
...
Producer: produced 0 (count=1)
...
Consumer: consumed 0 (count=0)
...
Producer: produced 19 (count=10)
Producer finished.
...
Consumer: consumed 10 (count=0)
Consumer finished.
Synchronization test completed

===== All Lab5 Tests Passed! =====

===== All Labs Complete =====
Total interrupts received: 1008
```

