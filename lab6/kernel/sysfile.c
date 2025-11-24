// kernel/sysfile.c
#include "defs.h"
#include "riscv.h" // [新增] 引入以使用 PTE_V, PGROUNDDOWN 等宏

// 引用外部定义的内核页表
extern pagetable_t kernel_pagetable;

// [新增] 辅助函数：验证虚拟地址范围是否有效
// 检查 [va, va+len) 范围内的所有页是否都在页表中已映射且有效
static int validate_addr(uint64 va, int len) {
    if (len < 0) return 0;
    uint64 start = va;
    uint64 end = va + len;
    if (end < start) return 0; // 防止溢出

    // 对范围内的每一页进行检查
    for (uint64 a = PGROUNDDOWN(start); a < end; a += PGSIZE) {
        pte_t *pte = walk_lookup(kernel_pagetable, a);
        
        // 如果 PTE 不存在，或者 PTE 的有效位 (V) 未置位
        if (pte == 0 || (*pte & PTE_V) == 0) {
            return 0; // 地址无效
        }
        
        // 可选：如果需要，还可以检查 PTE_R (读权限)
        // if ((*pte & PTE_R) == 0) return 0;
    }
    return 1; // 地址有效
}

int sys_write(void) {
    int fd;
    uint64 p;
    int n;

    if(argint(0, &fd) < 0 || argaddr(1, &p) < 0 || argint(2, &n) < 0)
        return -1;

    // [新增] 安全性检查：在访问指针 p 之前，先验证其有效性
    if (!validate_addr(p, n)) {
        // 如果地址无效，返回 -1 (错误)，而不是让内核崩溃
        return -1; 
    }

    // 简单实现：映射到控制台输出
    if (fd == 1 || fd == 2) {
        char *s = (char*)p;
        for(int i = 0; i < n; i++) {
            cons_putc(s[i]);
        }
        return n;
    }
    return -1;
}

int sys_read(void) { return 0; }
int sys_open(void) { return -1; }
int sys_close(void) { return 0; }