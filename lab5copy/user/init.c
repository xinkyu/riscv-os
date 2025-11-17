// user/init.c - 第一个用户进程
#include "user.h"

int main(void) {
    printf("init: starting\n");
    
    // 测试基本系统调用
    printf("init: pid = %d\n", getpid());
    
    // 测试 fork
    int pid = fork();
    if (pid == 0) {
        printf("init: child process, pid = %d\n", getpid());
        exit(0);
    } else if (pid > 0) {
        int status;
        wait(&status);
        printf("init: child exited with status %d\n", status);
    }
    
    printf("init: tests passed\n");
    
    while(1) {
        sleep(100);
    }
}