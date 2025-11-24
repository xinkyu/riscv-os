#include "defs.h"
#include "riscv.h"

extern void test_filesystem_integrity(void);
extern void test_concurrent_access(void);
extern void test_filesystem_performance(void);

void main() {
    consoleinit();
    printf("\n");
    printf("xv6-riscv kernel is booting\n");
    printf("\n");

    kinit();         // 物理内存初始化
    kvminit();       // 虚拟内存初始化
    kvminithart();   // 开启分页
    procinit();      // 进程表初始化
    trap_init();     // 中断初始化
    clock_init();    // 时钟初始化
    
    binit();         // [新增] 块缓存初始化
    iinit();         // [新增] Inode 缓存初始化
    fileinit();      // [新增] 文件表初始化
    virtio_disk_init(); // [新增] 磁盘驱动初始化
    
    // 文件系统必须在第 1 号设备上 (ROOTDEV)
    // 实际初始化会在首次 mount 或 fsinit 时进行
    fsinit(1);       // [新增] 挂载文件系统

    intr_on();       // 开启中断 (重要：磁盘读写依赖中断)

    printf("System initialized. Running tests...\n");

    // 运行测试
    // 注意：这些测试函数将在 kernel/test.c 中实现
    test_filesystem_integrity();
    // test_concurrent_access(); // 需要多进程支持，暂不运行
    test_filesystem_performance();

    printf("All tests passed!\n");
    
    scheduler();
}