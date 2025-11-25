// lab3/kernel/kalloc.c

#include "riscv.h"
#include "defs.h"

// 外部定义的内核结束地址
extern char end[];

// 空闲物理页链表的头节点
struct run {
    struct run *next;
};
static struct run *freelist;

// 初始化物理内存分配器
void kinit() {
    // 从内核末尾开始，直到物理内存顶部，将所有内存逐页释放
    // end 符号由链接脚本提供，表示内核镜像的结束位置
    // PHYSTOP 是 QEMU virt 机器的物理内存上限 (128MB)
    freerange(end, (void*)0x88000000); 
    printf("kinit: physical memory allocator initialized.\n");
}

// 将一段物理内存 [pa_start, pa_end) 添加到空闲链表
void freerange(void *pa_start, void *pa_end) {
    char *p = (char*)PGROUNDUP((uint64)pa_start);
    for (; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
        kfree(p);
    }
}

// 释放一个物理页
void kfree(void *pa) {
    struct run *r;

    // 检查地址是否合法
    if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= 0x88000000) {
        printf("kfree: invalid physical address %p\n", pa);
        return;
    }

    // 将页内存清零以防止信息泄露
    for (int i = 0; i < PGSIZE; i++) {
        *((char*)pa + i) = 1;
    }

    // 将释放的页加入空闲链表头部
    r = (struct run*)pa;
    r->next = freelist;
    freelist = r;
}

// 分配一个物理页
void *kalloc(void) {
    struct run *r = freelist;

    if (r) {
        freelist = r->next;
        // 将分配的页内存清零
        for (int i = 0; i < PGSIZE; i++) {
            *((char*)r + i) = 0;
        }
    }
    
    return (void*)r;
}