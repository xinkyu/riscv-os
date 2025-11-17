// lab4/kernel/main.c
#include "defs.h"
#include "riscv.h"

static inline uint64 r_sip() {
    uint64 x;
    asm volatile("csrr %0, sip" : "=r" (x));
    return x;
}

void kmain(void) {
    clear_screen();
    printf("===== Kernel Start =====\n");

    // 初始化物理内存分配器
    kinit();
    
    // 运行 Lab3 测试
    run_all_tests();
    
    // --- Lab4 新增 ---
    // 初始化中断向量
    trap_init();

    printf("\n[DEBUG] Before enabling interrupts:\n");
    printf("  sstatus = 0x%x\n", r_sstatus());
    printf("  sie = 0x%x\n", r_sie());
    printf("  sip = 0x%x\n", r_sip());
    
    printf("\n[Step 1] Enabling global interrupts...\n");
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    
    printf("[Step 2] Initializing clock (will call SBI to set timer)...\n");
    clock_init();
    
    printf("[Step 3] Clock initialized!\n");
    printf("  sie = 0x%x\n", r_sie());
    printf("  sip = 0x%x\n", r_sip());
    
    printf("\n[Step 4] Waiting for interrupts...\n");
    
    for (volatile long i = 0; i < 50000000L; i++) {
        if (i % 10000000 == 0) {
            printf("  [count=%d]\n", get_interrupt_count());
        }
    }
    
    printf("\n[Step 5] After waiting: %d interrupts received\n", get_interrupt_count());
    
    printf("\n===== Running Lab4 Tests =====\n\n");
    
    // 运行 Lab4 测试
    run_lab4_tests();
    
    printf("\n===== All Tests Complete =====\n");
    printf("Total interrupts received: %d\n", get_interrupt_count());
    printf("\nPress Ctrl-A then X to quit QEMU.\n");
    
    // 死循环
    while (1);
}