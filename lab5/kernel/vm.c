// lab4/kernel/vm.c

#include "riscv.h"
#include "defs.h"

// 内核的根页表
pagetable_t kernel_pagetable;

// 外部符号，由链接脚本定义
extern char etext[]; // .text 段的结束地址
extern char end[];   // 内核的结束地址

// 遍历页表，找到指定虚拟地址对应的PTE。
// 如果中间页表不存在且 alloc为1，则分配新页表。
static pte_t *walk(pagetable_t pagetable, uint64 va, int alloc) {
    if (va >= (1L << 39)) {
        return 0; // 虚拟地址过大
    }
    
    for (int level = 2; level > 0; level--) {
        pte_t *pte = &pagetable[VPN(va, level)];
        if (*pte & PTE_V) {
            pagetable = (pagetable_t)PTE2PA(*pte);
        } else {
            if (!alloc || (pagetable = (pagetable_t)kalloc()) == 0) {
                return 0; // 分配失败
            }
            for (int i = 0; i < PGSIZE / sizeof(pte_t); i++) {
                pagetable[i] = 0;
            }
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }
    return &pagetable[VPN(va, 0)];
}

// ===== 新增: 用于测试的公开函数 =====

// 创建一个新的空页表
pagetable_t create_pagetable(void) {
    pagetable_t pt = (pagetable_t)kalloc();
    if (pt == 0) {
        return 0;
    }
    // 清零
    for (int i = 0; i < PGSIZE / sizeof(pte_t); i++) {
        pt[i] = 0;
    }
    return pt;
}

// 映射单个页面
int map_page(pagetable_t pt, uint64 va, uint64 pa, int perm) {
    pte_t *pte = walk(pt, va, 1);
    if (pte == 0) {
        return -1;
    }
    if (*pte & PTE_V) {
        printf("map_page: remap is not supported\n");
        return -1;
    }
    *pte = PA2PTE(pa) | perm | PTE_V;
    return 0;
}

// 查找虚拟地址对应的PTE（不分配）
pte_t *walk_lookup(pagetable_t pt, uint64 va) {
    return walk(pt, va, 0);
}

// ===== 原有函数 =====

// 将一段虚拟地址映射到物理地址
int mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm) {
    uint64 a, last;
    pte_t *pte;

    a = PGROUNDDOWN(va);
    last = PGROUNDDOWN(va + size - 1);

    for (;;) {
        if ((pte = walk(pagetable, a, 1)) == 0) {
            return -1;
        }
        if (*pte & PTE_V) {
            printf("mappages: remap is not supported\n");
            return -1;
        }
        *pte = PA2PTE(pa) | perm | PTE_V;
        if (a == last) {
            break;
        }
        a += PGSIZE;
        pa += PGSIZE;
    }
    return 0;
}

// 创建内核页表
void kvminit(void) {
    kernel_pagetable = (pagetable_t)kalloc();
    for (int i = 0; i < PGSIZE / sizeof(pte_t); i++) {
        kernel_pagetable[i] = 0;
    }

    // 映射 UART 设备
    mappages(kernel_pagetable, 0x10000000, PGSIZE, 0x10000000, PTE_R | PTE_W);

    // 映射内核代码段 (R-X)
    mappages(kernel_pagetable, 0x80200000, (uint64)etext - 0x80200000, 0x80200000, PTE_R | PTE_X);

    // 映射内核数据段和剩余物理内存 (RW-)
    mappages(kernel_pagetable, (uint64)etext, 0x88000000 - (uint64)etext, (uint64)etext, PTE_R | PTE_W);
    
    printf("kvminit: kernel page table created.\n");
}

// 激活内核页表
void kvminithart(void) {
    w_satp(MAKE_SATP(kernel_pagetable));
    sfence_vma();
    printf("kvminithart: virtual memory enabled.\n");
}