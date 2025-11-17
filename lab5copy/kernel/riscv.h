// kernel/riscv.h (Lab6 修改版)
#ifndef __RISCV_H__
#define __RISCV_H__

typedef unsigned long uint64;
typedef unsigned int  uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef uint64 pte_t;
typedef uint64 *pagetable_t;

#define PGSIZE 4096
#define PGSHIFT 12

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)

#define PTE2PA(pte) (((pte) >> 10) << 12)
#define PTE_PA(pte) PTE2PA(pte)
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)
#define PTE_FLAGS(pte) ((pte) & 0x3FF)

#define VPN(va, level) (((uint64)(va) >> (12 + 9 * (level))) & 0x1FF)

// 虚拟内存布局
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))
#define TRAMPOLINE (MAXVA - PGSIZE)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)

#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

// sstatus 寄存器
#define SSTATUS_SPP (1L << 8)
#define SSTATUS_SPIE (1L << 5)
#define SSTATUS_SIE (1L << 1)

#define SIE_STIE (1L << 5)
#define SIE_SEIE (1L << 9)

static inline uint64 r_sstatus() {
    uint64 x;
    asm volatile("csrr %0, sstatus" : "=r" (x));
    return x;
}

static inline void w_sstatus(uint64 x) {
    asm volatile("csrw sstatus, %0" : : "r" (x));
}

static inline void w_stvec(uint64 x) {
    asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline uint64 r_sie() {
    uint64 x;
    asm volatile("csrr %0, sie" : "=r" (x));
    return x;
}

static inline void w_sie(uint64 x) {
    asm volatile("csrw sie, %0" : : "r" (x));
}

static inline uint64 r_scause() {
    uint64 x;
    asm volatile("csrr %0, scause" : "=r" (x));
    return x;
}

static inline uint64 r_sepc() {
    uint64 x;
    asm volatile("csrr %0, sepc" : "=r" (x));
    return x;
}

static inline void w_sepc(uint64 x) {
    asm volatile("csrw sepc, %0" : : "r" (x));
}

static inline uint64 r_stval() {
    uint64 x;
    asm volatile("csrr %0, stval" : "=r" (x));
    return x;
}

static inline uint64 r_satp() {
    uint64 x;
    asm volatile("csrr %0, satp" : "=r" (x));
    return x;
}

static inline void w_satp(uint64 x) {
    asm volatile("csrw satp, %0" : : "r" (x));
}

static inline void sfence_vma() {
    asm volatile("sfence.vma zero, zero");
}

static inline uint64 r_time() {
    uint64 x;
    asm volatile("csrr %0, time" : "=r" (x));
    return x;
}

static inline uint64 r_sp() {
    uint64 x;
    asm volatile("mv %0, sp" : "=r" (x));
    return x;
}

#endif // __RISCV_H__