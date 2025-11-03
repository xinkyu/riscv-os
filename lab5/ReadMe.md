# 实验五：进程管理与调度

## 实验目标

本实验的目标是为内核引入进程的概念，建立一个完整的进程管理框架，并实现一个基于时钟中断的抢占式轮转（Round-Robin）调度器。

## 核心实现

### 1. 进程抽象 (`kernel/proc.h`)

* **`struct context`**: 定义了进程切换时需要保存和恢复的 CPU 寄存器（`ra`, `sp` 和 `s0-s11`）。
* **`enum procstate`**: 定义了进程的生命周期状态，包括 `UNUSED`, `USED`, `SLEEPING`, `RUNNABLE`, `RUNNING`, `ZOMBIE`。
* **`struct proc`**: 定义了进程控制块（PCB），它包含了进程的状态、PID、内核栈、页表以及调度上下文 `context`。
* **`struct cpu`**: 用于支持多核（尽管本实验只用单核），存储当前CPU正在运行的进程和调度器自己的上下文。

### 2. 上下文切换 (`kernel/swtch.S`)

实现了 `swtch(old_ctx, new_ctx)` 函数。这是一个纯汇编函数，负责：
1. 将当前进程的 `ra`, `sp` 和 `s0-s11` 寄存器保存到 `old_ctx` 结构体中。
2. 从 `new_ctx` 结构体中加载下一个进程的寄存器。
3. 执行 `ret` 指令，跳转到 `new_ctx->ra` 所指向的地址，完成切换。

### 3. 进程管理 (`kernel/proc.c`)

* **`procinit`**: 初始化进程表，将所有 `proc` 结构体标记为 `UNUSED`。
* **`allocproc`**: 遍历进程表，查找一个 `UNUSED` 的进程。如果找到，为其分配 PID 和一个内核栈。
* **初始化上下文**: `allocproc` 将新进程的 `context.ra` 设置为 `forkret` 函数，`context.sp` 设置为其内核栈的栈顶。当 `swtch` 切换到该进程时，它将从 `forkret` 开始执行。
* **`userinit`**: 调用 `allocproc` 创建第一个内核态进程 `initproc`，并将其状态设置为 `RUNNABLE`。
* **`scheduler`**: 核心调度器循环。它不断地遍历进程表，查找状态为 `RUNNABLE` 的进程，并通过 `swtch` 切换到该进程执行。
* **`yield`**: 进程主动放弃 CPU。它将自身状态设为 `RUNNABLE`，然后调用 `sched` 切换到调度器。
* **`sleep` / `wakeup`**: 实现了基本的睡眠和唤醒原语，用于进程同步。

### 4. 抢占式调度 (`kernel/trap.c` 和 `kernel/main.c`)

* **`main` 函数**：在完成所有初始化后，不再是死循环，而是调用 `scheduler()` 函数，将控制权交给调度器。
* **`kerneltrap` 函数**：修改了时钟中断的处理逻辑。当中断发生时，除了打印 "tick"，还会调用 `yield()` 。这使得正在 `RUNNING` 的进程被强制放弃 CPU，重新变为 `RUNNABLE`，从而实现了抢占。

## 实验结果

执行 `make run` 后，内核启动，会完成所有初始化，并创建 `initproc`。调度器开始运行。我们可以观察到：
1. 终端周期性地打印 "tick: ..." 信息，表明时钟中断正常工作。
2. `initproc` 启动后打印 "initproc: starting..."。
3. 由于时钟中断不断触发 `yield`，`initproc` 和调度器（`scheduler`）的上下文在后台不断切换，CPU 在 `scheduler` 循环和 `initproc` 的 `yield` 循环之间交替执行，证明了抢占式调度成功运行。

