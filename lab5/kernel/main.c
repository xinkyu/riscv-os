// lab4/kernel/main.c -> lab5/kernel/main.c
#include "defs.h"
#include "riscv.h"

// 主任务，所有测试在这里运行
void main_task(void) {
    printf("===== main_task Started (PID %d) =====\n", myproc()->pid);

    // 运行 Lab3 测试
    run_all_tests();
    
    // 运行 Lab4 测试
    run_lab4_tests();

    // 运行 Lab5 测试
    run_lab5_tests();
    
    printf("\n===== All Labs Complete =====\n");
    printf("Total interrupts received: %d\n", get_interrupt_count());
    printf("\nPress Ctrl-A then X to quit QEMU.\n");
    
    // 停止
    while (1);
}

// 内核入口
void kmain(void) {
    clear_screen();
    printf("===== Kernel Booting =====\n");

    // 初始化
    kinit();
    kvminit();
    kvminithart();
    procinit();    // 初始化进程表
    trap_init();   // 初始化中断向量
    clock_init();  // 初始化时钟中断
    
    printf("\n[DEBUG] Creating main_task process...\n");
    
    // 创建第一个内核进程来运行测试
    if (create_process(main_task) < 0) {
        printf("kmain: failed to create main_task\n");
        while(1);
    }
    
    printf("[DEBUG] Starting scheduler...\n");
    
    // 启动调度器 (永不返回)
    scheduler();
}