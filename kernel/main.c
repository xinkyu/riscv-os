// kernel/main.c
#include "defs.h"

void kmain(void) {
    clear_screen(); // 清屏

    // 测试用例设计 
    printf("===== Begin printf test =====\n");
    printf("Testing basic integer: %d\n", 42); 
    printf("Testing negative integer: %d\n", -123); 
    printf("Testing zero: %d\n", 0); 
    printf("Testing hexadecimal: 0x%x\n", 0xABCDEF); 
    printf("Testing string: %s\n", "Hello, RISC-V!"); 
    printf("Testing percent sign: %%\n"); 

    // 边界条件测试 
    printf("\n===== Begin edge case test =====\n");
    printf("Testing INT_MAX: %d\n", 2147483647); 
    printf("Testing NULL string: %s\n", (char*)0); 
    printf("Testing empty string: %s\n", ""); 

    while (1);
}