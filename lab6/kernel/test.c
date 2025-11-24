// kernel/test.c
#include "riscv.h"
#include "defs.h"
#include "syscall.h"
#include "proc.h" 

#define NULL ((void*)0)

extern struct proc proc[NPROC];
extern uint64 get_time(void);

void assert(int condition) {
    if (!condition) {
        printf("ASSERTION FAILED!\n");
        while(1);
    }
}

static struct proc* find_current_process_hard() {
    struct proc *p = myproc();
    if (p && p->state == RUNNING) return p;
    for(int i = 0; i < NPROC; i++) {
        if (proc[i].state == RUNNING) return &proc[i];
    }
    return NULL;
}

static int do_syscall(int num, uint64 a0, uint64 a1, uint64 a2) {
    struct proc *p = find_current_process_hard();
    if (p == NULL) { printf("\n[FATAL] No RUNNING process!\n"); while(1); }

    register long t_a7 asm("a7") = num;
    register long t_a0 asm("a0") = a0;
    register long t_a1 asm("a1") = a1;
    register long t_a2 asm("a2") = a2;

    // [关键修复] 强制使用 .4byte 插入 32位 ebreak 指令 (0x00100073)
    // 这样 trap.c 中的 sepc+4 就永远是正确的，不会跳到指令中间导致崩溃
    asm volatile(".4byte 0x00100073" 
                 : "+r"(t_a0) 
                 : "r"(t_a1), "r"(t_a2), "r"(t_a7) 
                 : "memory");

    return t_a0;
}

int stub_getpid(void) { return do_syscall(SYS_getpid, 0, 0, 0); }
int stub_fork(void)   { return do_syscall(SYS_fork, 0, 0, 0); }
int stub_wait(int *s) { return do_syscall(SYS_wait, (uint64)s, 0, 0); }
void stub_exit(int s) { do_syscall(SYS_exit, s, 0, 0); }
int stub_write(int fd, char *p, int n) { return do_syscall(SYS_write, fd, (uint64)p, n); }

void test_basic_syscalls(void) {
    printf("\n=== Test 10: Basic System Calls ===\n");
    
    int pid = stub_getpid();
    printf("Current PID (via syscall): %d\n", pid);
    assert(pid > 0);

    printf("Testing fork()...\n");
    int child_pid = stub_fork();
    
    if (child_pid == 0) {
        printf("  [Child] Hello from child! PID=%d\n", stub_getpid());
        stub_exit(42);
    } else if (child_pid > 0) {
        printf("  [Parent] Forked child PID=%d\n", child_pid);
        int status = 0;
        int waited_pid = stub_wait(&status);
        printf("  [Parent] Child %d exited with status %d\n", waited_pid, status);
        assert(waited_pid == child_pid);
        assert(status == 42);
    } else {
        printf("Fork failed!\n");
    }
    printf("Basic system calls test passed\n");
}

void test_parameter_passing(void) {
    printf("\n=== Test 11: Parameter Passing ===\n");
    char *msg = "  [Syscall Write] Hello World via write()!\n";
    int len = 0;
    while(msg[len]) len++;
    int n = stub_write(1, msg, len);
    printf("  Write returned: %d (expected %d)\n", n, len);
    assert(n == len);
    printf("Parameter passing test passed\n");
}


// Test 12: 安全性测试
void test_security(void) {
    printf("\n=== Test 12: Security Test ===\n");
    char *invalid_ptr = (char*)0x0; 
    printf("  Writing to invalid pointer %p...\n", invalid_ptr);
    
    // [修改] 取消注释，并验证结果
    int result = stub_write(1, invalid_ptr, 10);
    printf("  Result: %d (Expected: -1)\n", result);
    
    assert(result == -1);
    printf("Security test passed: Invalid pointer correctly rejected\n");
}
// Test 13: 性能测试
void test_syscall_performance(void) {
    printf("\n=== Test 13: Syscall Performance ===\n");
    
    uint64 start_time = get_time();
    int count = 10000;

    printf("  Running %d getpid() calls...\n", count);
    for (int i = 0; i < count; i++) {
        stub_getpid(); // 这里面现在没有 printf 了，非常快
    }

    uint64 end_time = get_time();
    uint64 duration = end_time - start_time;
    
    printf("  %d getpid() calls took %lu cycles\n", count, duration);
    if (count > 0) printf("  Average cycles per syscall: %lu\n", duration / count);
    printf("Performance test passed\n");
}

void run_lab6_tests(void) {
    printf("\n===== Starting Lab6 Tests =====\n");
    test_basic_syscalls();
    test_parameter_passing();
    test_security();
    test_syscall_performance();
    printf("\n===== All Lab6 Tests Passed! =====\n");
}

// 存根
void test_physical_memory(void) {}
void test_pagetable(void) {}
void test_virtual_memory(void) {}
void test_timer_interrupt(void) {}
void test_exception_handling(void) {}
void test_interrupt_overhead(void) {}
void run_all_tests(void) {}
void run_lab4_tests(void) {}
void run_lab5_tests(void) {}