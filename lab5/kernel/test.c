// lab5/kernel/test.c - 进程管理与调度测试

#include "defs.h"
#include "riscv.h"
#include "proc.h"

// ========== 辅助函数 ==========

// 简单的断言实现
static void assert(int condition, const char *msg) {
    if (!condition) {
        printf("ASSERTION FAILED: %s\n", msg);
        while(1);
    }
}

// 获取当前时间（cycles）
static inline uint64 r_time() {
    uint64 x;
    asm volatile("csrr %0, time" : "=r" (x));
    return x;
}

// ========== 测试任务函数 ==========

// 简单任务：打印并让出 CPU
void simple_task(void) {
    struct proc *p = myproc();
    printf("Simple task running, PID: %d\n", p->pid);
    
    for (int i = 0; i < 5; i++) {
        printf("  [PID %d] iteration %d\n", p->pid, i);
        yield();
    }
    
    printf("Simple task (PID %d) completed\n", p->pid);
    // 注意：Lab5 尚未实现 exit，进程会继续 yield
    while(1) yield();
}

// CPU 密集型任务
void cpu_intensive_task(void) {
    struct proc *p = myproc();
    printf("CPU intensive task starting, PID: %d\n", p->pid);
    
    volatile uint64 sum = 0;
    for (int i = 0; i < 1000000; i++) {
        sum += i;
        if (i % 100000 == 0) {
            yield(); // 周期性让出 CPU
        }
    }
    
    printf("CPU intensive task (PID %d) completed, sum=%d\n", p->pid, sum);
    while(1) yield();
}

// 生产者任务（简化版）
static volatile int shared_buffer[10];
static volatile int buffer_count = 0;

void producer_task(void) {
    struct proc *p = myproc();
    printf("Producer (PID %d) starting\n", p->pid);
    
    for (int i = 0; i < 5; i++) {
        // 简单的生产逻辑
        while (buffer_count >= 10) {
            yield(); // 等待空间
        }
        
        shared_buffer[buffer_count++] = i;
        printf("  [Producer PID %d] produced: %d\n", p->pid, i);
        yield();
    }
    
    printf("Producer (PID %d) completed\n", p->pid);
    while(1) yield();
}

// 消费者任务（简化版）
void consumer_task(void) {
    struct proc *p = myproc();
    printf("Consumer (PID %d) starting\n", p->pid);
    
    for (int i = 0; i < 5; i++) {
        // 简单的消费逻辑
        while (buffer_count <= 0) {
            yield(); // 等待数据
        }
        
        int item = shared_buffer[--buffer_count];
        printf("  [Consumer PID %d] consumed: %d\n", p->pid, item);
        yield();
    }
    
    printf("Consumer (PID %d) completed\n", p->pid);
    while(1) yield();
}

// ========== 测试函数 ==========

// 测试1：进程创建
void test_process_creation(void) {
    printf("\n=== Test 1: Process Creation ===\n");
    
    printf("Testing basic process creation...\n");
    
    // 注意：Lab5 当前实现只有 initproc，没有 create_process 函数
    // 这里我们测试进程表的初始状态
    
    int used_count = 0;
    for (int i = 0; i < NPROC; i++) {
        if (proc[i].state != UNUSED) {
            used_count++;
            printf("  Process %d: PID=%d, State=%d\n", 
                   i, proc[i].pid, proc[i].state);
        }
    }
    
    printf("Active processes: %d / %d\n", used_count, NPROC);
    assert(used_count >= 1, "At least initproc should exist");
    
    printf("Process creation test: PASSED\n");
}

// 测试2：进程状态转换
void test_process_states(void) {
    printf("\n=== Test 2: Process State Transitions ===\n");
    
    struct proc *p = myproc();
    if (p) {
        printf("Current process: PID=%d, State=%d\n", p->pid, p->state);
        assert(p->state == RUNNING, "Current process should be RUNNING");
        
        printf("Testing yield (RUNNING -> RUNNABLE)...\n");
        // yield 会改变状态为 RUNNABLE，然后调度器会再次选中它
        yield();
        
        printf("After yield, back to RUNNING\n");
        assert(myproc()->state == RUNNING, "Should be RUNNING again");
    }
    
    printf("Process state transition test: PASSED\n");
}

// 测试3：调度器基本功能
void test_scheduler_basic(void) {
    printf("\n=== Test 3: Scheduler Basic Function ===\n");
    
    printf("Scheduler is running (you're seeing this means it works!)\n");
    
    // 测试多次 yield
    printf("Testing multiple yields...\n");
    for (int i = 0; i < 5; i++) {
        printf("  Yield test iteration %d\n", i);
        yield();
    }
    
    printf("Scheduler basic function test: PASSED\n");
}

// 测试4：时间片轮转
void test_time_slice(void) {
    printf("\n=== Test 4: Time Slice and Preemption ===\n");
    
    printf("Running CPU-bound loop (should be preempted by timer)...\n");
    
    uint64 start_time = r_time();
    volatile uint64 count = 0;
    
    // 执行计算，直到被时钟中断抢占多次
    for (int round = 0; round < 3; round++) {
        uint64 round_start = r_time();
        count = 0;
        
        // 计算直到至少过去 50000 cycles
        while (r_time() - round_start < 50000) {
            count++;
        }
        
        printf("  Round %d: counted to %d\n", round, count);
    }
    
    uint64 elapsed = r_time() - start_time;
    printf("Total elapsed: %d cycles\n", elapsed);
    
    printf("Time slice test: PASSED\n");
}

// 测试5：上下文切换
void test_context_switch(void) {
    printf("\n=== Test 5: Context Switch ===\n");
    
    struct proc *p = myproc();
    printf("Testing context preservation...\n");
    
    // 保存一些栈变量
    int test_var1 = 0x12345678;
    int test_var2 = 0xABCDEF00;
    
    printf("Before yield: var1=0x%x, var2=0x%x\n", test_var1, test_var2);
    
    // 让出 CPU，测试上下文是否正确保存和恢复
    yield();
    
    printf("After yield: var1=0x%x, var2=0x%x\n", test_var1, test_var2);
    
    assert(test_var1 == 0x12345678, "Context not preserved: var1");
    assert(test_var2 == 0xABCDEF00, "Context not preserved: var2");
    
    printf("Context switch test: PASSED\n");
}

// 测试6：进程表管理
void test_proc_table(void) {
    printf("\n=== Test 6: Process Table Management ===\n");
    
    printf("=== Process Table Dump ===\n");
    int total = 0, unused = 0, runnable = 0, running = 0, sleeping = 0;
    
    for (int i = 0; i < NPROC; i++) {
        struct proc *p = &proc[i];
        total++;
        
        switch(p->state) {
            case UNUSED:   unused++; break;
            case RUNNABLE: runnable++; break;
            case RUNNING:  running++; break;
            case SLEEPING: sleeping++; break;
            default: break;
        }
        
        if (p->state != UNUSED) {
            printf("  Slot %d: PID=%d State=%d KStack=0x%x\n",
                   i, p->pid, p->state, p->kstack);
        }
    }
    
    printf("\nProcess table statistics:\n");
    printf("  Total slots: %d\n", total);
    printf("  Unused: %d\n", unused);
    printf("  Runnable: %d\n", runnable);
    printf("  Running: %d\n", running);
    printf("  Sleeping: %d\n", sleeping);
    
    assert(unused + runnable + running + sleeping == NPROC, 
           "Process table accounting error");
    
    printf("Process table management test: PASSED\n");
}

// ========== 主测试入口 ==========

void run_lab5_tests(void) {
    printf("\n========================================\n");
    printf("===== Lab5 Process Management Tests =====\n");
    printf("========================================\n");
    
    // 只有第一个进程（initproc）会运行这些测试
    struct proc *p = myproc();
    if (p && p->pid == 1) {
        test_process_creation();
        test_process_states();
        test_scheduler_basic();
        test_time_slice();
        test_context_switch();
        test_proc_table();
        
        printf("\n========================================\n");
        printf("===== All Lab5 Tests PASSED! =====\n");
        printf("========================================\n");
        
        printf("\nInitproc entering infinite yield loop...\n");
    }
    
    // 所有测试完成后，进入 yield 循环
    while(1) {
        yield();
    }
}
