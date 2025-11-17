// kernel/main.c (Lab6 修改版)
#include "defs.h"
#include "riscv.h"

void main_task(void) {
    printf("===== main_task Started (PID %d) =====\n", myproc()->pid);

    run_all_tests();
    run_lab4_tests();
    run_lab5_tests();
    
    // Lab6: 创建第一个用户进程
    printf("\n===== Starting Lab6 =====\n");
    printf("Creating first user process...\n");
    
    if (userinit() < 0) {
        printf("userinit failed\n");
        while(1);
    }
    
    // 运行 Lab6 测试
    run_lab6_tests();
    
    printf("\n===== All Labs Complete =====\n");
    printf("Total interrupts: %d\n", get_interrupt_count());
    
    while (1) {
        sleep(get_ticks_channel(), &myproc()->lock);
    }
}

void kmain(void) {
    clear_screen();
    printf("===== Kernel Booting =====\n");

    kinit();
    kvminit();
    kvminithart();
    procinit();
    trap_init();
    usertrapinit();
    clock_init();
    
    printf("\n[DEBUG] Creating main_task...\n");
    
    if (create_process(main_task) < 0) {
        printf("Failed to create main_task\n");
        while(1);
    }
    
    printf("[DEBUG] Starting scheduler...\n");
    scheduler();
}