// lab3/kernel/main.c
#include "defs.h"

void kmain(void) {
    clear_screen(); // 清屏
    printf("===== Kernel Start =====\n");

    // 初始化物理内存分配器
    kinit();
    
    // 选项1: 运行测试（测试会内部调用 kvminit 和 kvminithart）
    run_all_tests();
    
    // 选项2: 如果不想运行测试，保持原来的初始化
    // kvminit();
    // kvminithart();
    // printf("\nInitialization complete. Entering halt state.\n");

    // 内核初始化完成，进入死循环
    while (1);
}