// kernel/vm.c (Lab6 修改版)
#include "riscv.h"
#include "defs.h"

pagetable_t kernel_pagetable;
extern char etext[];
extern char trampoline[];

static pte_t *walk(pagetable_t pagetable, uint64 va, int alloc) {
    if (va >= (1L << 39)) {
        return 0;
    }
    
    for (int level = 2; level > 0; level--) {
        pte_t *pte = &pagetable[VPN(va, level)];
        if (*pte & PTE_V) {
            pagetable = (pagetable_t)PTE2PA(*pte);
        } else {
            if (!alloc || (pagetable = (pagetable_t)kalloc()) == 0) {
                return 0;
            }
            memset(pagetable, 0, PGSIZE);
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }
    return &pagetable[VPN(va, 0)];
}

pagetable_t create_pagetable(void) {
    pagetable_t pt = (pagetable_t)kalloc();
    if (pt == 0) {
        return 0;
    }
    memset(pt, 0, PGSIZE);
    return pt;
}

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

pte_t *walk_lookup(pagetable_t pt, uint64 va) {
    return walk(pt, va, 0);
}

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
            printf("mappages: remap\n");
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

void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free) {
    uint64 a;
    pte_t *pte;

    for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
        if((pte = walk(pagetable, a, 0)) == 0)
            continue;
        if((*pte & PTE_V) == 0)
            continue;
        if(PTE_FLAGS(*pte) == PTE_V)
            printf("uvmunmap: not a leaf");
        if(do_free){
            uint64 pa = PTE2PA(*pte);
            kfree((void*)pa);
        }
        *pte = 0;
    }
}

// 复制父进程的内存到子进程
int uvmcopy(pagetable_t old, pagetable_t new, uint64 sz) {
    pte_t *pte;
    uint64 pa, i;
    uint flags;
    char *mem;

    for(i = 0; i < sz; i += PGSIZE){
        if((pte = walk(old, i, 0)) == 0)
            continue;
        if((*pte & PTE_V) == 0)
            continue;
        pa = PTE2PA(*pte);
        flags = PTE_FLAGS(*pte);
        if((mem = kalloc()) == 0)
            goto err;
        memmove(mem, (char*)pa, PGSIZE);
        if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
            kfree(mem);
            goto err;
        }
    }
    return 0;

err:
    uvmunmap(new, 0, i / PGSIZE, 1);
    return -1;
}

// 分配用户内存
uint64 uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz) {
    char *mem;
    uint64 a;

    if(newsz < oldsz)
        return oldsz;

    oldsz = PGROUNDUP(oldsz);
    for(a = oldsz; a < newsz; a += PGSIZE){
        mem = kalloc();
        if(mem == 0){
            uvmunmap(pagetable, oldsz, (a - oldsz) / PGSIZE, 1);
            return 0;
        }
        memset(mem, 0, PGSIZE);
        if(mappages(pagetable, a, PGSIZE, (uint64)mem, PTE_W|PTE_R|PTE_U) != 0){
            kfree(mem);
            uvmunmap(pagetable, oldsz, (a - oldsz) / PGSIZE, 1);
            return 0;
        }
    }
    return newsz;
}

// 从内核复制到用户空间
int copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len) {
    uint64 n, va0, pa0;
    pte_t *pte;

    while(len > 0){
        va0 = PGROUNDDOWN(dstva);
        if(va0 >= MAXVA)
            return -1;
        pte = walk(pagetable, va0, 0);
        if(pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_U) == 0)
            return -1;
        pa0 = PTE2PA(*pte);
        n = PGSIZE - (dstva - va0);
        if(n > len)
            n = len;
        memmove((void *)(pa0 + (dstva - va0)), src, n);

        len -= n;
        src += n;
        dstva = va0 + PGSIZE;
    }
    return 0;
}

// 从用户空间复制到内核
int copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len) {
    uint64 n, va0, pa0;
    pte_t *pte;

    while(len > 0){
        va0 = PGROUNDDOWN(srcva);
        if(va0 >= MAXVA)
            return -1;
        pte = walk(pagetable, va0, 0);
        if(pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_U) == 0)
            return -1;
        pa0 = PTE2PA(*pte);
        n = PGSIZE - (srcva - va0);
        if(n > len)
            n = len;
        memmove(dst, (void *)(pa0 + (srcva - va0)), n);

        len -= n;
        dst += n;
        srcva = va0 + PGSIZE;
    }
    return 0;
}

// 从用户空间复制字符串到内核
int copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max) {
    uint64 n, va0, pa0;
    int got_null = 0;
    pte_t *pte;

    while(got_null == 0 && max > 0){
        va0 = PGROUNDDOWN(srcva);
        if(va0 >= MAXVA)
            return -1;
        pte = walk(pagetable, va0, 0);
        if(pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_U) == 0)
            return -1;
        pa0 = PTE2PA(*pte);
        n = PGSIZE - (srcva - va0);
        if(n > max)
            n = max;

        char *p = (char *) (pa0 + (srcva - va0));
        while(n > 0){
            if(*p == '\0'){
                *dst = '\0';
                got_null = 1;
                break;
            } else {
                *dst = *p;
            }
            --n;
            --max;
            p++;
            dst++;
        }

        srcva = va0 + PGSIZE;
    }
    if(got_null){
        return 0;
    } else {
        return -1;
    }
}

void kvminit(void) {
    kernel_pagetable = (pagetable_t)kalloc();
    memset(kernel_pagetable, 0, PGSIZE);

    mappages(kernel_pagetable, 0x10000000, PGSIZE, 0x10000000, PTE_R | PTE_W);
    mappages(kernel_pagetable, 0x80200000, (uint64)etext - 0x80200000, 0x80200000, PTE_R | PTE_X);
    mappages(kernel_pagetable, (uint64)etext, 0x88000000 - (uint64)etext, (uint64)etext, PTE_R | PTE_W);
    
    printf("kvminit: kernel page table created.\n");
}

void kvminithart(void) {
    w_satp(MAKE_SATP(kernel_pagetable));
    sfence_vma();
    printf("kvminithart: virtual memory enabled.\n");
}

// 为进程创建用户页表
pagetable_t proc_pagetable(struct proc *p) {
    pagetable_t pagetable;

    pagetable = create_pagetable();
    if(pagetable == 0)
        return 0;

    // 映射 trampoline（暂时跳过，简化实现）
    // 映射 trapframe
    if(mappages(pagetable, TRAPFRAME, PGSIZE,
                (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
        uvmunmap(pagetable, 0, 0, 0);
        kfree((void*)pagetable);
        return 0;
    }

    return pagetable;
}

void proc_freepagetable(pagetable_t pagetable, uint64 sz) {
    uvmunmap(pagetable, 0, PGROUNDUP(sz)/PGSIZE, 1);
    uvmunmap(pagetable, TRAPFRAME, 1, 0);
    kfree((void*)pagetable);
}