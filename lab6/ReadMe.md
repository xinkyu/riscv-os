# 实验六：系统调用 (System Calls)

## 实验目标

本实验的目标是为内核搭建一个完整的“系统调用”框架。这是连接用户态程序和内核态服务的桥梁。通过 `ecall` 指令，未来的用户程序将能够请求内核执行特权操作，例如获取进程ID或读写文件。

## 核心实现

### 1. 陷阱处理 (`kernel/trap.c`, `kernel/kernelvec.S`)

* **`kernelvec.S`**: 修改了汇编入口点，使其在保存所有寄存器到栈上后，将栈指针（`sp`，即 `trapframe` 的地址）作为第一个参数（`a0`）传递给 C 语言的 `kerneltrap` 函数。
* **`kernel/trap.c`**: 修改了 `kerneltrap` 函数，使其能够检查 `scause` 寄存器。当 `scause` 为 `8` 时，意味着这是一次来自用户态的 `ecall`。
* **`sepc` 处理**: 在 `scause == 8` 的分支中，我们将 `sepc`（异常程序计数器）加 4。这确保了当 `sret` 返回时，程序会从 `ecall` 的*下一条*指令继续执行，而不是再次陷入 `ecall` 死循环。

### 2. `trapframe` 结构体 (`kernel/proc.h`)

* 定义了 `struct trapframe`。这个 C 结构体的字段顺序**严格匹配** `kernelvec.S` 中寄存器的保存顺序。
* 这使得 `kerneltrap` (C代码) 能够轻松地通过 `tf->a7` 访问系统调用号，或通过 `tf->a0` 访问参数。
* 修改了 `allocproc`，使 `proc->trapframe` 指针指向内核栈的顶部，为陷阱帧预留了空间。

### 3. 系统调用分发器 (`kernel/syscall.c`, `kernel/syscall.h`)

* **`syscall.h`**: 创建了新文件，用于定义所有系统调用的编号（如 `SYS_getpid`, `SYS_write` 等）。
* **`syscall.c`**: 创建了核心的分发机制：
    * `syscalls[]`: 一个函数指针数组，将系统调用编号映射到其实际的内核实现函数。
    * `syscall()`: `kerneltrap` 调用的总入口。它读取 `tf->a7` 中的编号，在数组中查找对应的函数并执行，最后将返回值存入 `tf->a0`。

### 4. 系统调用实现 (`kernel/sysproc.c`)

* 创建了新文件，用于存放 `sys_` 系列函数的具体实现。
* 作为第一个实现，添加了 `sys_getpid()`，它通过 `myproc()` 获取当前进程的 PID。

### 5. 关键并发问题修复 (Debugging)

在集成实验六时，暴露了实验五调度器中的多个并发 Bug，本次实验一并进行了修复：

* **`printf` 死锁**: 修复了 `printf`（在 `scheduler` 中）被 `printf`（在 `kerneltrap` 中）重入导致的死锁。通过在 `printf` 函数体前后添加 `intr_off()` 和 `w_sstatus(old_sstatus)`，确保了 `printf` 的原子性。
* **调度器活锁**: 修复了 `kmain` 过早开启中断导致 `scheduler` 无法执行的问题。将 `w_sstatus(..|SIE)` 调用从 `kmain` 移至 `forkret`，确保第一个进程启动后才开始响应时钟中断。
* **`sched()` 状态机 Bug**: 修复了 `yield` 将状态设为 `RUNNABLE` 后 `sched` 却检查 `RUNNING` 导致的调度失败。`sched()` 现在被修改为无条件切换上下文。

## 实验结果

* 由于目前还没有加载用户态程序的能力，我们无法通过 `ecall` 测试完整的陷阱路径。
* **验证**：在 `forkret` (第一个内核进程 `initproc` 的入口) 中，**直接调用**内核函数 `sys_getpid()`。
* **结果**：内核成功启动，调度器正常运行，`initproc` 打印出 `[Lab6 Test] SUCCESS! sys_getpid is linked.`。
* 在修复了并发 Bug 后，系统不再打印 "sched: process not running"，并且在 `initproc` 和 `scheduler` 之间稳定地进行上下文切换，成功在空闲状态下响应时钟中断（打印 "tick"）。