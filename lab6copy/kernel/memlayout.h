// kernel/memlayout.h
#ifndef __MEMLAYOUT_H__
#define __MEMLAYOUT_H__

// Physical memory layout for qemu -machine virt

// UART0 串口基地址
#define UART0 0x10000000L
#define UART0_IRQ 10

// VirtIO 磁盘设备基地址
#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

// CLINT (Core Local Interruptor)
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

// PLIC (Platform Level Interrupt Controller)
#define PLIC 0x0c000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

// 内核起始物理地址
#define KERNBASE 0x80000000L
// 物理内存结束地址 (假设 128MB RAM)
#define PHYSTOP (KERNBASE + 128*1024*1024)

// 虚拟地址空间布局
// 蹦床页面 (Trampoline) 映射在虚拟地址最高处
// MAXVA = 1L << (9 + 9 + 9 + 12 - 1) = 0x3fffffffff
// 注意：必须与 riscv.h 中的定义一致
#ifndef MAXVA
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))
#endif

#ifndef PGSIZE
#define PGSIZE 4096
#endif

#define TRAMPOLINE (MAXVA - PGSIZE)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)

#endif