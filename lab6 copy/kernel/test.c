#include "defs.h"
#include "fcntl.h"
#include "file.h"
#include "stat.h"
#include "riscv.h"
#include "fs.h"
#include "param.h"

// ================== Kernel Internal Wrappers ==================
// 这些函数模拟了系统调用的行为，但是直接在内核中运行，不依赖 copyin/copyout

// 简易的内核文件描述符表，用于测试
struct file *test_open_files[NOFILE]; 

int kopen(char *path, int omode) {
    struct inode *ip;
    struct file *f;
    int fd;

    // 1. Find free slot
    for(fd = 0; fd < NOFILE; fd++){
        if(test_open_files[fd] == 0) break;
    }
    if(fd >= NOFILE) return -1;

    begin_op();
    if(omode & O_CREATE){
        ip = create(path, T_FILE, 0, 0); // 假设 kernel/sysfile.c 中的 create 移除了 static，或者在这里重新实现
        if(ip == 0){ end_op(); return -1; }
    } else {
        if((ip = namei(path)) == 0){ end_op(); return -1; }
        ilock(ip);
        if(ip->type == T_DIR && omode != O_RDONLY){
            iunlockput(ip); end_op(); return -1;
        }
    }

    if((f = filealloc()) == 0){
        iunlockput(ip); end_op(); return -1;
    }

    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

    iunlock(ip);
    end_op();

    test_open_files[fd] = f;
    return fd;
}

int kwrite(int fd, void *addr, int n) {
    if(fd < 0 || fd >= NOFILE || test_open_files[fd] == 0) return -1;
    // 直接调用 filewrite，因为 addr 是内核地址，所以 fileread/write 需要能处理
    // 我们在 fs.c readi/writei 中添加了 user_src 参数，这里传 0 表示内核地址
    return filewrite(test_open_files[fd], (uint64)addr, n); 
}

int kread(int fd, void *addr, int n) {
    if(fd < 0 || fd >= NOFILE || test_open_files[fd] == 0) return -1;
    return fileread(test_open_files[fd], (uint64)addr, n);
}

int kclose(int fd) {
    if(fd < 0 || fd >= NOFILE || test_open_files[fd] == 0) return -1;
    fileclose(test_open_files[fd]);
    test_open_files[fd] = 0;
    return 0;
}

int kunlink(char *path) {
    // 简化的 unlink，不处理所有的安全检查
    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ], p[MAXPATH];
    uint off;

    safestrcpy(p, path, MAXPATH);
    begin_op();
    if((dp = nameiparent(p, name)) == 0){ end_op(); return -1; }
    ilock(dp);
    if((ip = dirlookup(dp, name, &off)) == 0){ iunlockput(dp); end_op(); return -1; }
    ilock(ip);
    memset(&de, 0, sizeof(de));
    writei(dp, 0, (uint64)&de, off, sizeof(de));
    iunlockput(dp);
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    end_op();
    return 0;
}

// 映射宏，让下面的测试代码不用修改
#define open kopen
#define write kwrite
#define read kread
#define close kclose
#define unlink kunlink
#define assert(x) if(!(x)){ printf("ASSERT FAILED: %s\n", #x); while(1); }

// ================== 指南书测试代码 ==================

void test_filesystem_integrity(void) {
    printf("Testing filesystem integrity...\n");
    
    // 确保之前的文件已被清理
    unlink("testfile");

    // 创建测试文件
    int fd = open("testfile", O_CREATE | O_RDWR);
    assert(fd >= 0);

    // 写入数据
    char buffer[] = "Hello, filesystem!";
    int bytes = write(fd, buffer, strlen(buffer));
    assert(bytes == strlen(buffer));
    close(fd);

    // 重新打开并验证
    fd = open("testfile", O_RDONLY);
    assert(fd >= 0);

    char read_buffer[64];
    bytes = read(fd, read_buffer, sizeof(read_buffer));
    read_buffer[bytes] = '\0';

    assert(strncmp(buffer, read_buffer, 64) == 0); // 使用 strncmp 代替 strcmp
    close(fd);

    // 删除文件
    assert(unlink("testfile") == 0);

    printf("Filesystem integrity test passed\n");
}

void test_filesystem_performance(void) {
    printf("Testing filesystem performance...\n");
    uint64 start_time = get_time();

    // 大量小文件测试
    for (int i = 0; i < 50; i++) { // 稍微减小规模以免 QEMU 跑太久
        char filename[32];
        // 简单的 snprintf 替代
        char *digits = "0123456789";
        int t = i;
        filename[0] = 's'; filename[1] = '_'; 
        if(t < 10) { filename[2] = digits[t]; filename[3] = 0; }
        else { filename[2] = digits[t/10]; filename[3] = digits[t%10]; filename[4] = 0; }

        int fd = open(filename, O_CREATE | O_RDWR);
        if(fd >= 0) {
            write(fd, "test", 4);
            close(fd);
        }
    }

    uint64 small_files_time = get_time() - start_time;

    // 清理
    for (int i = 0; i < 50; i++) {
        char filename[32];
        char *digits = "0123456789";
        int t = i;
        filename[0] = 's'; filename[1] = '_'; 
        if(t < 10) { filename[2] = digits[t]; filename[3] = 0; }
        else { filename[2] = digits[t/10]; filename[3] = digits[t%10]; filename[4] = 0; }
        unlink(filename);
    }

    printf("Small files test completed in %d cycles\n", small_files_time);
}

void run_all_tests() {
    // 空函数，防止链接错误，测试已在 main 中调用
}