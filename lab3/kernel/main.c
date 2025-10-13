// lab3/kernel/main.c
#include "defs.h"

void kmain(void) {
    clear_screen(); // 清屏
    printf("===== Kernel Start =====\n");

    // 初始化物理内存分配器
    kinit();
    
    // 创建并初始化内核页表
    kvminit();
    
    // 启用虚拟内存
    kvminithart();

    printf("\nInitialization complete. Entering halt state.\n");
    
    // 内核初始化完成，进入死循环
    while (1);
}