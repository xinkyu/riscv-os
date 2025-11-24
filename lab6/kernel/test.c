// kernel/test.c
#include "riscv.h"
#include "defs.h"
#include "syscall.h"
#include "proc.h" // 必须包含，以便访问 struct proc 定义

#define NULL ((void*)0)

// 引用全局进程表
extern struct proc proc[NPROC];

void assert(int condition) {
    if (!condition) {
        printf("ASSERTION FAILED!\n");
        while(1);
    }
}

// 查找当前进程的辅助函数 (直接遍历数组)
static struct proc* find_current_process_hard() {
    for(int i = 0; i < NPROC; i++) {
        if (proc[i].state == RUNNING) {
            return &proc[i];
        }
    }
    return NULL;
}

// 模拟系统调用存根
static int do_syscall(int num, uint64 a0, uint64 a1, uint64 a2) {
    // 1. 直接在 proc 数组中找到自己
    struct proc *p = find_current_process_hard();
    
    // 2. 严厉的检查
    if (p == NULL) {
        printf("\n[FATAL] do_syscall: Could not find ANY process in RUNNING state!\n");
        // 如果找不到自己，说明状态维护彻底崩了，无法继续
        while(1); 
    }

    if (p->trapframe == NULL) {
        printf("\n[FATAL] PID %d has NULL trapframe!\n", p->pid);
        while(1);
    }

    // 3. 填充参数到 trapframe
    p->trapframe->a7 = num;
    p->trapframe->a0 = a0;
    p->trapframe->a1 = a1;
    p->trapframe->a2 = a2;
    
    // 4. 执行 ecall
    asm volatile("ecall");
    
    // 5. 返回结果
    return p->trapframe->a0;
}

// 避免与内核函数重名
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

void run_lab6_tests(void) {
    printf("\n===== Starting Lab6 Tests =====\n");
    test_basic_syscalls();
    test_parameter_passing();
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