// kernel/console.c
#include "defs.h"

#include "file.h"
// 控制台输出一个字符
// 目前它只是简单地调用 UART 驱动
void cons_putc(char c) {
    uart_putc(c);
}
// [新增] 适配文件系统的读写函数
int consoleread(int user_dst, uint64 dst, int n) {
  // 简单的非阻塞读取实现，实际 OS 需要等待输入缓冲
  // 这里暂时返回 0 (EOF) 以跑通文件系统测试
  return 0; 
}

int consolewrite(int user_src, uint64 src, int n) {
  int i;
  char c;
  for(i = 0; i < n; i++){
    // 直接从内核地址读，忽略 user_src 标志（简化）
    c = *((char*)src + i); 
    uart_putc(c);
  }
  return n;
}

void consoleinit(void) {
  initlock(&cons.lock, "cons");
  
  // [新增] 注册控制台设备到设备开关表
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
  
  uartinit();
}