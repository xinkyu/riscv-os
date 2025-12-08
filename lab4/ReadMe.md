# 实验四：中断处理与时钟管理

**姓名**：
**学号**：
**日期**：2025-12-08
---

## 一、实验概述

### 实验目标
- 依照《从零构建操作系统-学生指导手册》实验4任务，构建完整的陷阱/中断子系统，使内核能够在 S-mode 下接收 OpenSBI 委托的时钟中断并进行调度前的计时管理。
- 梳理 csrs（sstatus/sie/sip/scause/stvec）与 `kernelvec`/`kerneltrap`/`clock_init` 的协作流程，为后续中断驱动的调度做好铺垫。

### 完成情况
- ✅ 任务1：理解 RISC-V 中断委托机制，绘制 M-mode→S-mode 委托链并在 `main.c` 中打印关键 CSR。
- ✅ 任务2/3：构建中断控制接口（`trap_init/clock_init`）并实现 tick 计数、SIE/STIE 使能和 `sbi_set_timer` 封装。
- ✅ 任务4：根据手册要求实现 `kernelvec.S`，保存/恢复 32 个寄存器，并用 `sret` 返回。
- ✅ 任务5：实现时钟中断逻辑，确保 `kerneltrap` 能安全更新 `tick_counter` 并重设下一次 timer。
- ✅ 任务6：搭建异常处理骨架，保留测试钩子于 `test_exception_handling` 中。
- ✅ 测试：`run_lab4_tests()` 覆盖计时、异常框架确认、中断开销测量，运行日志记录于 `out.txt`。

### 开发环境
- OS：Ubuntu 22.04 LTS
- 工具链：riscv64-unknown-elf-gcc 12.2.0，binutils 2.40
- 模拟器：qemu-system-riscv64 8.2.0（virt，10 MHz ACLINT Timer）
- 其他：GNU Make 4.3，OpenSBI v1.5.1

---

## 二、技术设计

### 系统架构
```text
┌────────────┐   ┌────────────┐   ┌────────────────────────┐
│ entry.S    │ → │ main.c     │ → │ Trap 子系统             │
└────────────┘   └────────────┘   │ kernelvec.S (任务4)     │
                                  │ trap.c: trap_init/     │
                                  │   clock_init/kerneltrap│
                                  │ test.c: Lab4 测试套件   │
                                  └───────────┬────────────┘
                                              │ S-mode 中断
                                     ┌────────▼────────┐
                                     │ OpenSBI Timer   │
                                     │ (mideleg/medeleg│
                                     │ 任务1)           │
                                     └─────────────────┘
```

| 模块 | 与 xv6 的区别 | 设计理由 |
| --- | --- | --- |
| `kernelvec.S` | xv6 另外保存 `satp` 等寄存器并支持多核；本实验仅读写单核 32 个 GPR | 任务4 仅要求单核上下文保存，保持实现最小化以降低调试难度 |
| `trap.c` | xv6 使用 `struct trapframe` + `swtch`；此处仅聚焦内核态中断 | Lab4 尚未涉及用户态/上下文切换，简化数据结构便于展示核心思路 |
| `clock_init` | xv6 调用 platform-specific CLINT；本实验通过 SBI `set_timer` | QEMU virt + OpenSBI 环境下推荐使用 SBI 接口，满足任务5“理解SBI时钟”要求 |

### 关键数据结构与寄存器

```c
static volatile uint64 tick_counter;       // 任务5：记录时钟中断次数
static volatile uint64 total_interrupt_count;

#define SIE_STIE (1 << 5)                  // 启用 S-mode timer interrupt

static inline void sbi_set_timer(uint64 stime) {
    register uint64 a7 asm("a7") = 0;     // SBI Timer Extension
    register uint64 a6 asm("a6") = 0;     // set_timer 功能号
    register uint64 a0 asm("a0") = stime; // 下一次触发时间
    asm volatile("ecall" : "+r"(a0) : "r"(a6), "r"(a7) : "memory");
}
```
- `tick_counter`/`total_interrupt_count` 供测试框架查询，避免在中断内打印。
- CSR 组合：`sstatus.SIE` 控制全局使能；`sie.STIE` 控制时钟源；`sip.STIP` 只读指示待处理中断；`stvec` 指向 `kernelvec`；`scause` 用于判断“同步/异步 + 中断号”。

### 核心流程
1. **初始化阶段（任务2/3）**
   - `trap_init` → `w_stvec((uint64)kernelvec)`，保证入口 16 字节对齐。
   - `clock_init`：先 `sbi_set_timer(r_time()+100000)` 清除 STIP，再置位 `sie.STIE`。
2. **中断入口（任务4）**
   - `kernelvec` 使用 256 B 栈空间保存 32 个 GPR，调用 `kerneltrap`，返回后按逆序恢复并 `sret`。
3. **时钟中断处理（任务5）**
   - `kerneltrap` 判断 `scause` 最高位；若为中断且 ID=5，则累加计数并重新设置下一次 timer；其它情况暂以死循环代替异常处理。
4. **异常框架（任务6）**
   - `test_exception_handling` 预留触发代码，当前仅验证框架存在，避免对中断稳定性造成影响。

---

## 三、实现细节与关键代码

### 1. 关键函数片段

```asm
# kernel/kernelvec.S (节选)
.align 4
kernelvec:
    addi sp, sp, -256
    sd ra, 0(sp)
    ...              # 32 个寄存器依次保存
    call kerneltrap
    ...              # 按相反顺序恢复
    addi sp, sp, 256
    sret
```
- 采用栈式方案符合指导手册“保存到内核栈”建议，后续可扩展为 per-hart 栈。

```c
// kernel/trap.c (节选)
void kerneltrap(void) {
    uint64 scause = r_scause();
    if (scause & (1L << 63)) {
        uint64 cause = scause & 0x7FFFFFFFFFFFFFFF;
        if (cause == 5) {            // S-mode timer interrupt
            total_interrupt_count++;
            tick_counter++;
            sbi_set_timer(r_time() + 100000);
        }
    } else {
        while (1);                   // 异常处理留给任务6后续拓展
    }
}
```
- 避免中断内 `printf`，通过 `get_interrupt_count()` 在普通上下文查询，解决手册 FAQ 中“串口冲突”问题。

```c
// kernel/main.c (节选)
trap_init();
w_sstatus(r_sstatus() | SSTATUS_SIE);
clock_init();
printf("[Step 4] Waiting for interrupts...\n");
for (volatile long i = 0; i < 5e7; i++) {
    if (i % 10000000 == 0) {
        printf("  [count=%d]\n", get_interrupt_count());
    }
}
run_lab4_tests();
```
- 通过分步日志确认初始化顺序：先向量表→全局 SIE→时钟源→忙等待观测。

### 2. 难点与解决

| 难点 | 现象 | 原因分析 | 解决思路 |
| --- | --- | --- | --- |
| STIP 置位导致首次 `clock_init` 立即返回 | `sip=0x20`、`kerneltrap` 连续触发 | OpenSBI 开机时已启动 timer，S-mode 无法直接清 STIP | 在 `clock_init` 前半段立即调用 `sbi_set_timer`，由 OpenSBI 清除 pending 位 |
| 中断内调 `printf` 卡死 | 内核在 `kerneltrap` 中打印 tick，串口出现 re-entry | UART 驱动非中断安全，被中断打断时重入 | 去除中断内输出，改在主循环/测试中查询计数 |
| 大量寄存器保存导致栈污染 | 初版忘记恢复 `sp` 保存值 | `sd sp,8(sp)` 破坏了现有栈指针 | 不保存 sp 本身，仅在入口 `addi sp,-256`、出口 `addi sp,+256`，保持对称 |

### 3. 对手册思考题的简答
- **为何时钟中断要先在 M-mode 处理**：硬件计时器由 ACLINT/CLINT 驱动，需 M-mode 配置；通过 `mideleg`/`medeleg` 将中断/异常委托给 S-mode，保证安全与兼容性。
- **如何扩展优先级/嵌套**：当前 `kerneltrap` 不做嵌套处理；若后续支持，可在保存上下文后根据 `scause` 号查表、允许高优先级中断在 `sstatus.SIE` 置位时抢占。

---

## 四、测试与验证

### 1. 功能测试矩阵

| 测试 | 手册任务 | 关注点 | 结果 |
| --- | --- | --- | --- |
| `test_physical_memory`/`test_pagetable`/`test_virtual_memory` | 继承实验3 | 回归内存管理避免回归 | ✅ 通过|
| `test_timer_interrupt` | 任务4 | `get_interrupt_count` 连续递增、间隔≈100k cycles | ✅ 通过（详见输出） |
| `test_exception_handling` | 任务5 | 验证异常处理占位，提示如何触发 | ✅ 通过（默认跳过破坏性用例） |
| `test_interrupt_overhead` | 任务6 (性能) | 在中断启用状态下测计算耗时 | ✅ 通过 |

### 2. 输出摘录
摘自 `lab4/out.txt`：

```
trap_init: stvec set to 0x000000008020098c
clock_init: supervisor timer interrupt enabled.

[Step 1] Enabling global interrupts...
[Step 2] Initializing clock (will call SBI to set timer)...
...
[Step 5] After waiting: 20 interrupts received

=== Test 4: Timer Interrupt ===
Start time: 4533115, Start interrupt count: 21
End time:   5063576, End interrupt count: 26
✓ Timer test completed: 5 interrupts in 530461 cycles

=== Test 6: Interrupt Overhead ===
Computation with interrupts:
  Elapsed cycles: 20273
  Interrupts occurred: 0
  Result: 499999500000 (to prevent optimization)

===== All Lab4 Tests Passed! =====
Total interrupts received: 26
```
### 3. 运行截图

![Lab4 测试通过截图](lab4-boot-success.png)

### 4. 运行/调试方法

```bash
cd lab4
make clean && make run        # 构建并运行全部测试
```

退出 QEMU：`Ctrl-A` → `X`。如需观察中断触发过程，可在第二个终端执行：

```bash
make qemu-gdb                 # 启动等待 GDB 的 QEMU
riscv64-unknown-elf-gdb kernel.elf
(gdb) b kerneltrap
(gdb) c
```

---

## 五、问题与总结

### 1. 遇到的问题
1. **待处理中断无法清除**：STIP 只读导致“刚开机就连续进中断”。通过“先 `sbi_set_timer` 再启用 STIE”让 OpenSBI 自动清标志。
2. **中断重入引发串口死锁**：在 `kerneltrap` 中打印 tick 触发重入，改为全局计数器+查询接口解决。
3. **上下文恢复遗漏寄存器**：早期版本漏恢复 `s0/s1` 导致返回后栈帧损坏；通过 checklist 逐项核对 32 个寄存器并启用 `run_lab4_tests` 验证。

### 2. 实验收获
- 系统掌握了 RISC-V 中断委托链条：OpenSBI（M-mode）→ S-mode `stvec` → `kerneltrap`。
- 熟悉了 SBI 调用约定（`a7` 拓展号、`a6` 功能号、`a0` 参数），能够利用 `r_time()` + `sbi_set_timer()` 精准控制中断频率。
- 认识到“中断上下文必须极简”的工程约束，并学会使用全局计数/延时循环来验证时钟。

### 3. 改进方向
- **异常细分**：实现 `handle_exception(struct trapframe*)`，对非法指令/访存异常给出具体报错。
- **中断优先级**：仿照手册任务3思考题，引入简单优先级表和嵌套屏蔽，减少高频 timer 对其他中断的影响。
- **调度器集成**：在 `kerneltrap` 中检测 tick，达到阈值后触发调度，为 lab5 进程/线程实验打基础。

---


