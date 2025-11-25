// kernel/main.c
#include "defs.h"
#include "riscv.h"

void main_task(void) {
    printf("===== main_task Started (PID %d) =====\n", myproc()->pid);
    KLOG_INFO("main", "main_task entered (pid=%d)", myproc()->pid);
    run_lab6_tests();
    run_lab7_tests();
    run_lab8_tests();
    printf("\n===== All Labs Complete =====\n");
    printf("Press Ctrl-A then X to quit QEMU.\n");
    KLOG_INFO("main", "all labs finished; entering idle loop");
    while (1);
}

void kmain(void) {
    clear_screen();
    printf("===== Kernel Booting =====\n");
    kinit();
    kvminit();
    kvminithart();
    procinit();
    trap_init();
    clock_init();
    binit();
    fileinit();
    virtio_disk_init();
    iinit();
    initlog(ROOTDEV, &sb);
    klog_init();
    KLOG_INFO("boot", "subsystems initialized, starting user workload");
    
    if (create_process(main_task) < 0) {
        printf("kmain: failed to create main_task\n");
        KLOG_FATAL("boot", "failed to spawn main_task");
        while(1);
    }
    scheduler();
}