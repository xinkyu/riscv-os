// kernel/test.c
#include "riscv.h"
#include "defs.h"
#include "syscall.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"
#include "stat.h"

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
int stub_read(int fd, void *p, int n) { return do_syscall(SYS_read, fd, (uint64)p, n); }
int stub_open(const char *path, int mode) { return do_syscall(SYS_open, (uint64)path, mode, 0); }
int stub_close(int fd) { return do_syscall(SYS_close, fd, 0, 0); }
int stub_unlink(const char *path) { return do_syscall(SYS_unlink, (uint64)path, 0, 0); }
int stub_mkdir(const char *path) { return do_syscall(SYS_mkdir, (uint64)path, 0, 0); }
int stub_chdir(const char *path) { return do_syscall(SYS_chdir, (uint64)path, 0, 0); }
int stub_dup(int fd) { return do_syscall(SYS_dup, fd, 0, 0); }
int stub_link(const char *old, const char *newp) { return do_syscall(SYS_link, (uint64)old, (uint64)newp, 0); }
int stub_mknod(const char *path, int major, int minor) { return do_syscall(SYS_mknod, (uint64)path, major, minor); }
int stub_fstat(int fd, struct stat *st) { return do_syscall(SYS_fstat, fd, (uint64)st, 0); }

static void build_name(char *buf, const char *prefix, int idx) {
    int i = 0;
    while (prefix[i]) {
        buf[i] = prefix[i];
        i++;
    }
    char digits[16];
    int n = 0;
    if (idx == 0) {
        digits[n++] = '0';
    } else {
        while (idx > 0 && n < (int)sizeof(digits)) {
            digits[n++] = '0' + (idx % 10);
            idx /= 10;
        }
    }
    while (n > 0) {
        buf[i++] = digits[--n];
    }
    buf[i] = '\0';
}

static void test_filesystem_integrity(void);
static void test_concurrent_access(void);
static void test_crash_recovery(void);
static void test_filesystem_performance(void);
static void debug_filesystem_state(void);
static void debug_inode_usage(void);
static void debug_disk_io(void);

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

void test_security(void) {
    printf("\n=== Test 12: Security Test ===\n");
    char *invalid_ptr = (char*)0x0; 
    printf("  Writing to invalid pointer %p...\n", invalid_ptr);
    
    int result = stub_write(1, invalid_ptr, 10);
    printf("  Result: %d (Expected: -1)\n", result);
    
    assert(result == -1);
    printf("Security test passed: Invalid pointer correctly rejected\n");
}

void test_syscall_performance(void) {
    printf("\n=== Test 13: Syscall Performance ===\n");
    
    uint64 start_time = get_time();
    int count = 10000;

    printf("  Running %d getpid() calls...\n", count);
    for (int i = 0; i < count; i++) {
        stub_getpid(); 
    }

    uint64 end_time = get_time();
    uint64 duration = end_time - start_time;
    
    printf("  %d getpid() calls took %lu cycles\n", count, duration);
    if (count > 0) printf("  Average cycles per syscall: %lu\n", duration / count);
    printf("Performance test passed\n");
}

void run_lab6_tests(void) {
    printf("\n===== Starting Lab6 Tests =====\n");
    
    // 尝试打开一个文件作为 stdout，如果尚未打开
    int fd = stub_open("console", O_CREATE | O_RDWR); 
    if (fd == 0) {
        stub_dup(fd); // fd 1
        stub_dup(fd); // fd 2
    }

    test_basic_syscalls();
    test_parameter_passing();
    test_security();
    test_syscall_performance();
    printf("\n===== All Lab6 Tests Passed! =====\n");
}

void run_lab7_tests(void) {
    printf("\n===== Starting Lab7 Filesystem Tests =====\n");
    test_filesystem_integrity();
    test_concurrent_access();
    test_crash_recovery();
    test_filesystem_performance();
    debug_filesystem_state();
    debug_inode_usage();
    debug_disk_io();
    printf("===== Lab7 Filesystem Tests Completed =====\n");
}

// === 增强调试信息的完整性测试 ===
static void test_filesystem_integrity(void) {
    printf("\n=== FS Test 1: Integrity ===\n");
    char buffer[] = "Hello, filesystem!";
    char read_buffer[64];

    printf("  1. Opening file for write...\n");
    int fd = stub_open("testfile", O_CREATE | O_RDWR);
    if (fd < 0) {
        printf("  FAIL: Open failed, fd=%d\n", fd);
        assert(0);
    }
    printf("  Open success, fd=%d\n", fd);

    printf("  2. Writing data...\n");
    int written = stub_write(fd, buffer, strlen(buffer));
    if (written != strlen(buffer)) {
        printf("  FAIL: Write failed, written=%d, expected=%d\n", written, strlen(buffer));
        assert(0);
    }
    printf("  Write success\n");
    
    stub_close(fd);

    printf("  3. Re-opening for read...\n");
    fd = stub_open("testfile", O_RDONLY);
    if (fd < 0) {
        printf("  FAIL: Re-open failed, fd=%d\n", fd);
        assert(0);
    }
    printf("  Re-open success, fd=%d\n", fd);

    printf("  4. Reading data...\n");
    // 清空缓冲区，防止读取到垃圾数据
    for(int k=0; k<64; k++) read_buffer[k] = 0;
    
    int bytes = stub_read(fd, read_buffer, sizeof(read_buffer) - 1);
    if (bytes != strlen(buffer)) {
        printf("  FAIL: Read count mismatch, bytes=%d, expected=%d\n", bytes, strlen(buffer));
        assert(0);
    }
    read_buffer[bytes] = '\0';
    printf("  Read bytes: %d, Content: '%s'\n", bytes, read_buffer);

    if (strncmp(buffer, read_buffer, bytes) != 0) {
        printf("  FAIL: Content mismatch!\n");
        printf("  Expected: '%s'\n", buffer);
        printf("  Actual:   '%s'\n", read_buffer);
        assert(0);
    }
    
    stub_close(fd);
    
    printf("  5. Unlinking...\n");
    if (stub_unlink("testfile") != 0) {
        printf("  FAIL: Unlink failed\n");
        assert(0);
    }
    
    printf("Filesystem integrity test passed\n");
}

static void test_concurrent_access(void) {
    printf("\n=== FS Test 2: Concurrent Access ===\n");
    const int workers = 4;
    const int iterations = 100;
    for (int i = 0; i < workers; i++) {
        int pid = stub_fork();
        if (pid == 0) {
            char filename[32];
            build_name(filename, "test_", i);
            for (int j = 0; j < iterations; j++) {
                int fd = stub_open(filename, O_CREATE | O_RDWR);
                if (fd >= 0) {
                    stub_write(fd, (char*)&j, sizeof(j));
                    stub_close(fd);
                    stub_unlink(filename);
                }
            }
            stub_exit(0);
        }
    }
    for (int i = 0; i < workers; i++) {
        int status = 0;
        stub_wait(&status);
    }
    printf("Concurrent access test completed\n");
}

static void test_crash_recovery(void) {
    printf("\n=== FS Test 3: Crash Recovery (Simulated) ===\n");
    char data[BSIZE];
    for (int i = 0; i < BSIZE; i++) {
        data[i] = (char)(i & 0xff);
    }

    int fd = stub_open("crashfile", O_CREATE | O_RDWR);
    assert(fd >= 0);
    assert(stub_write(fd, data, sizeof(data)) == sizeof(data));
    stub_close(fd);

    // 模拟重启：重新初始化日志并恢复
    initlog(ROOTDEV, &sb);

    fd = stub_open("crashfile", O_RDONLY);
    assert(fd >= 0);
    char verify[BSIZE];
    int bytes = stub_read(fd, verify, sizeof(verify));
    assert(bytes == sizeof(verify));
    assert(memcmp(data, verify, sizeof(data)) == 0);
    stub_close(fd);
    stub_unlink("crashfile");
    printf("Crash recovery simulation passed\n");
}

static void test_filesystem_performance(void) {
    printf("\n=== FS Test 4: Performance ===\n");
    uint64 start = get_time();
    const int small_files = 200;
    char filename[32];
    for (int i = 0; i < small_files; i++) {
        build_name(filename, "small_", i);
        int fd = stub_open(filename, O_CREATE | O_RDWR);
        if (fd >= 0) {
            stub_write(fd, "test", 4);
            stub_close(fd);
        }
    }
    uint64 small_time = get_time() - start;

    start = get_time();
    int fd = stub_open("large_file", O_CREATE | O_RDWR | O_TRUNC);
    assert(fd >= 0);
    char buffer[4096];
    for (int i = 0; i < (1024); i++) {
        stub_write(fd, buffer, sizeof(buffer));
    }
    stub_close(fd);
    uint64 large_time = get_time() - start;

    printf("Small files (%d x 4B): %lu cycles\n", small_files, small_time);
    printf("Large file (4MB): %lu cycles\n", large_time);

    for (int i = 0; i < small_files; i++) {
        build_name(filename, "small_", i);
        stub_unlink(filename);
    }
    stub_unlink("large_file");
}

static void debug_filesystem_state(void) {
    struct superblock info;
    get_superblock(&info);
    printf("\n=== Filesystem Debug Info ===\n");
    printf("Total blocks: %d\n", info.size);
    printf("Data blocks: %d\n", info.nblocks);
    printf("Free blocks: %d\n", count_free_blocks());
    printf("Free inodes: %d\n", count_free_inodes());
    printf("Buffer cache hits: %lu\n", get_buffer_cache_hits());
    printf("Buffer cache misses: %lu\n", get_buffer_cache_misses());
}

static void debug_inode_usage(void) {
    dump_inode_usage();
}

static void debug_disk_io(void) {
    printf("=== Disk I/O Statistics ===\n");
    printf("Disk reads: %lu\n", get_disk_read_count());
    printf("Disk writes: %lu\n", get_disk_write_count());
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