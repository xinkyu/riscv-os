// lab3/kernel/riscv.h

#ifndef __RISCV_H__
#define __RISCV_H__

// 定义常用的类型
typedef unsigned long uint64;
typedef uint64 pte_t;      // 页表项
typedef uint64 *pagetable_t; // 页表

//
// RISC-V Sv39 虚拟内存系统相关定义
//
#define PGSIZE 4096 // 每个页的大小 (4KB)
#define PGSHIFT 12  // log2(PGSIZE)

// 将地址向下/向上对齐到页边界
#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

// 页表项 (PTE) 中的标志位
#define PTE_V (1L << 0) // Valid: 有效位
#define PTE_R (1L << 1) // Read: 可读
#define PTE_W (1L << 2) // Write: 可写
#define PTE_X (1L << 3) // Execute: 可执行
#define PTE_U (1L << 4) // User: 用户态可访问

// 从 PTE 中提取物理页号 (PPN)
// PPN 占 PTE 的高44位 (53:10)
#define PTE2PA(pte) (((pte) >> 10) << 12)

// 从物理地址构建 PTE
// PA 占 PTE 的高44位
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)

// 提取虚拟地址的各级页表索引
#define VPN(va, level) (((uint64)(va) >> (12 + 9 * (level))) & 0x1FF)

//
// Supervisor Address Translation and Protection (SATP) 寄存器
//
#define SATP_SV39 (8L << 60) // 设置 SATP 寄存器使用 Sv39 分页模式
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

//
// 用于读写 RISC-V 控制寄存器的内联汇编函数
//

// 读取 satp 寄存器
static inline uint64 r_satp() {
    uint64 x;
    asm volatile("csrr %0, satp" : "=r" (x));
    return x;
}

// 写入 satp 寄存器
static inline void w_satp(uint64 x) {
    asm volatile("csrw satp, %0" : : "r" (x));
}

// 刷新 TLB (Translation Lookaside Buffer)
static inline void sfence_vma() {
    asm volatile("sfence.vma zero, zero");
}

#endif // __RISCV_H__