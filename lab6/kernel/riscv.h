// kernel/riscv.h

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
#define PTE2PA(pte) (((pte) >> 10) << 12)

// 从物理地址构建 PTE
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)

// 提取虚拟地址的各级页表索引
#define VPN(va, level) (((uint64)(va) >> (12 + 9 * (level))) & 0x1FF)

//
// Supervisor Address Translation and Protection (SATP) 寄存器
//
#define SATP_SV39 (8L << 60) // 设置 SATP 寄存器使用 Sv39 分页模式
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

//
// --- Lab4/5 新增：中断和异常相关的 CSR 定义 ---
//

// sstatus (Supervisor Status Register)
#define SSTATUS_SIE (1L << 1) // Supervisor Interrupt Enable bit
#define SSTATUS_SPP (1L << 8) // Supervisor Previous Privilege (0=U, 1=S)

// sie (Supervisor Interrupt Enable Register)
#define SIE_STIE (1L << 5) // Supervisor Timer Interrupt Enable bit

//
// 用于读写 RISC-V 控制寄存器的内联汇编函数
//

// 读取 sstatus 寄存器
static inline uint64 r_sstatus() {
    uint64 x;
    asm volatile("csrr %0, sstatus" : "=r" (x));
    return x;
}

// 写入 sstatus 寄存器
static inline void w_sstatus(uint64 x) {
    asm volatile("csrw sstatus, %0" : : "r" (x));
}

// 写入 stvec (Supervisor Trap Vector Base Address) 寄存器
static inline void w_stvec(uint64 x) {
    asm volatile("csrw stvec, %0" : : "r" (x));
}

// 读取 sie (Supervisor Interrupt Enable) 寄存器
static inline uint64 r_sie() {
    uint64 x;
    asm volatile("csrr %0, sie" : "=r" (x));
    return x;
}

// 写入 sie 寄存器
static inline void w_sie(uint64 x) {
    asm volatile("csrw sie, %0" : : "r" (x));
}

// 读取 scause (Supervisor Cause) 寄存器
static inline uint64 r_scause() {
    uint64 x;
    asm volatile("csrr %0, scause" : "=r" (x));
    return x;
}

// 读取 sepc (Supervisor Exception Program Counter) 寄存器
static inline uint64 r_sepc() {
    uint64 x;
    asm volatile("csrr %0, sepc" : "=r" (x));
    return x;
}

// 写入 sepc 寄存器
static inline void w_sepc(uint64 x) {
    asm volatile("csrw sepc, %0" : : "r" (x));
}

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

// --- Lab6 新增：陷阱帧 ---
// 
// 这是一个简化的、类似 xv6 的陷阱帧结构。
// 它用于在从用户态陷入内核态时，保存所有用户寄存器。
//
struct trapframe {
    /* 0 */ uint64 ra;
    /* 8 */ uint64 sp;
    /* 16 */ uint64 gp;
    /* 24 */ uint64 tp;
    /* 32 */ uint64 t0;
    /* 40 */ uint64 t1;
    /* 48 */ uint64 t2;
    /* 56 */ uint64 s0;
    /* 64 */ uint64 s1;
    /* 72 */ uint64 a0; // 系统调用返回值
    /* 80 */ uint64 a1;
    /* 88 */ uint64 a2;
    /* 96 */ uint64 a3;
    /* 104 */ uint64 a4;
    /* 112 */ uint64 a5;
    /* 120 */ uint64 a6;
    /* 128 */ uint64 a7; // a7 用于存放系统调用号
    /* 136 */ uint64 s2;
    /* 144 */ uint64 s3;
    /* 152 */ uint64 s4;
    /* 160 */ uint64 s5;
    /* 168 */ uint64 s6;
    /* 176 */ uint64 s7;
    /* 184 */ uint64 s8;
    /* 192 */ uint64 s9;
    /* 200 */ uint64 s10;
    /* 208 */ uint64 s11;
    /* 216 */ uint64 t3;
    /* 224 */ uint64 t4;
    /* 232 */ uint64 t5;
    /* 240 */ uint64 t6;
    /* 248 */ uint64 epc; // 保存 sepc
    
    // 以下字段由内核填充，用于陷阱返回
    /* 256 */ uint64 kernel_sp;     // 内核栈顶指针
    /* 264 */ uint64 kernel_trap;   // kerneltrap() 的地址
    /* 272 */ uint64 kernel_satp;   // 内核页表
};


#endif // __RISCV_H__