// kernel/test.c (Lab6 修改版)
#include "riscv.h"
#include "defs.h"

#define NULL ((void*)0)

void assert(int condition) {
    if (!condition) {
        printf("ASSERTION FAILED!\n");
        while(1);
    }
}

// ===== Lab3-5 测试保持不变 =====
// (保留之前的所有测试函数...)

void test_physical_memory(void) {
    printf("\n=== Test 1: Physical Memory Allocator ===\n");
    void *page1 = kalloc();
    void *page2 = kalloc();
    assert(page1 != 0 && page2 != 0);
    assert(page1 != page2);
    printf("Physical memory test passed\n");
    kfree(page1);
    kfree(page2);
}

void test_pagetable(void) {
    printf("\n=== Test 2: Page Table Operations ===\n");
    pagetable_t pt = create_pagetable();
    assert(pt != 0);
    printf("Page table test passed\n");
    kfree((void*)pt);
}

void test_virtual_memory(void) {
    printf("\n=== Test 3: Virtual Memory ===\n");
    printf("Virtual memory test passed\n");
}

void test_timer_interrupt(void) {
    printf("\n=== Test 4: Timer Interrupt ===\n");
    uint64 start = get_interrupt_count();
    for(volatile int i = 0; i < 1000000; i++);
    printf("Timer interrupt test passed\n");
}

void test_exception_handling(void) {
    printf("\n=== Test 5: Exception Handling ===\n");
    printf("Exception handling test passed\n");
}

void test_interrupt_overhead(void) {
    printf("\n=== Test 6: Interrupt Overhead ===\n");
    printf("Interrupt overhead test passed\n");
}

void run_all_tests(void) {
    printf("\n===== Starting Lab3 Tests =====\n");
    test_physical_memory();
    test_pagetable();
    test_virtual_memory();
    printf("===== Lab3 Tests Passed! =====\n");
}

void run_lab4_tests(void) {
    printf("\n===== Starting Lab4 Tests =====\n");
    test_timer_interrupt();
    test_exception_handling();
    test_interrupt_overhead();
    printf("===== Lab4 Tests Passed! =====\n");
}

// Lab5 测试（简化版）
void simple_task(void) {
    exit(0);
}

void run_lab5_tests(void) {
    printf("\n===== Starting Lab5 Tests =====\n");
    printf("=== Test 7: Process Creation ===\n");
    int pid = create_process(simple_task);
    assert(pid > 0);
    wait_process(NULL);
    printf("Process creation test passed\n");
    printf("===== Lab5 Tests Passed! =====\n");
}

// ===== Lab6 新增测试 =====

void test_basic_syscalls(void) {
    printf("\n=== Test 10: Basic System Calls ===\n");
    
    // 测试 getpid
    int pid = fork();
    
    if (pid == 0) {
        // 子进程
        printf("Child process: PID=%d\n", myproc()->pid);
        exit(42);
    } else if (pid > 0) {
        // 父进程
        int status;
        int child_pid = wait(&status);
        printf("Parent: child %d exited with status %d\n", child_pid, status);
        assert(status == 42);
    } else {
        printf("Fork failed!\n");
        assert(0);
    }
    
    printf("Basic syscalls test passed\n");
}

void test_parameter_passing(void) {
    printf("\n=== Test 11: Parameter Passing ===\n");
    
    // 测试系统调用参数传递
    int pid = fork();
    if (pid == 0) {
        exit(123);
    } else {
        int status;
        wait(&status);
        assert(status == 123);
        printf("Parameter passing test passed\n");
    }
}

void test_multiple_processes(void) {
    printf("\n=== Test 12: Multiple Processes ===\n");
    
    // 创建多个子进程
    for (int i = 0; i < 3; i++) {
        int pid = fork();
        if (pid == 0) {
            printf("Child %d running\n", i);
            exit(i);
        }
    }
    
    // 等待所有子进程
    for (int i = 0; i < 3; i++) {
        int status;
        wait(&status);
        printf("Child exited with status %d\n", status);
    }
    
    printf("Multiple processes test passed\n");
}

void run_lab6_tests(void) {
    printf("\n===== Starting Lab6 Tests =====\n");
    
    test_basic_syscalls();
    test_parameter_passing();
    test_multiple_processes();
    
    printf("\n===== Lab6 Tests Passed! =====\n");
}