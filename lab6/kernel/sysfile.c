// kernel/sysfile.c
#include "defs.h"

int sys_write(void) {
    int fd;
    uint64 p;
    int n;

    if(argint(0, &fd) < 0 || argaddr(1, &p) < 0 || argint(2, &n) < 0)
        return -1;

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
int sys_open(void) { return -1; } // 总是失败或返回假fd
int sys_close(void) { return 0; }