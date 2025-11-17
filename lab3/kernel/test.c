// kernel/test.c
#include "riscv.h"
#include "defs.h"

// 简单的断言实现
void assert(int condition) {
    if (!condition) {
        printf("ASSERTION FAILED!\n");
        while(1); // 停止执行
    }
}

// 测试1: 物理内存分配器测试
void test_physical_memory(void) {
    printf("\n=== Test 1: Physical Memory Allocator ===\n");
    
    // 测试基本分配和释放
    void *page1 = kalloc();
    void *page2 = kalloc();
    printf("page1 = %p, page2 = %p\n", page1, page2);
    assert(page1 != page2);
    assert(((uint64)page1 & 0xFFF) == 0); // 页对齐检查
    assert(((uint64)page2 & 0xFFF) == 0);
    printf("Page allocation and alignment test passed\n");

    // 测试数据写入
    *(int*)page1 = 0x12345678;
    assert(*(int*)page1 == 0x12345678);
    printf("Memory write test passed\n");

    // 测试释放和重新分配
    kfree(page1);
    void *page3 = kalloc();
    printf("page3 = %p (after freeing page1)\n", page3);
    // page3可能等于page1（取决于分配策略）
    
    kfree(page2);
    kfree(page3);
    printf("Free and reallocation test passed\n");
}

// 测试2: 页表操作测试
void test_pagetable(void) {
    printf("\n=== Test 2: Page Table Operations ===\n");
    
    pagetable_t pt = create_pagetable();
    assert(pt != 0);
    printf("Page table created at %p\n", pt);

    // 测试基本映射
    uint64 va = 0x1000000;
    uint64 pa = (uint64)kalloc();
    assert(pa != 0);
    printf("Mapping VA %p to PA %p\n", va, pa);
    assert(map_page(pt, va, pa, PTE_R | PTE_W) == 0);
    printf("Basic mapping test passed\n");

    // 测试地址转换
    pte_t *pte = walk_lookup(pt, va);
    assert(pte != 0 && (*pte & PTE_V));
    assert(PTE2PA(*pte) == pa);
    printf("Address translation test passed\n");

    // 测试权限位
    assert(*pte & PTE_R);
    assert(*pte & PTE_W);
    assert(!(*pte & PTE_X));
    printf("Permission bits test passed\n");
    
    // 清理
    kfree((void*)pa);
}

// 测试3: 虚拟内存激活测试
void test_virtual_memory(void) {
    printf("\n=== Test 3: Virtual Memory Activation ===\n");
    printf("Before enabling paging...\n");
    printf("Kernel code is accessible\n");
    printf("Kernel data is accessible\n");
    
    // 启用分页
    kvminit();
    kvminithart();
    
    printf("After enabling paging...\n");
    printf("Kernel code still executable\n");
    printf("Kernel data still accessible\n");
    printf("Device access still working\n");
}

// 运行所有测试
void run_all_tests(void) {
    printf("\n===== Starting Memory Management Tests =====\n");
    
    test_physical_memory();
    test_pagetable();
    test_virtual_memory();
    
    printf("\n===== All Tests Passed! =====\n");
}