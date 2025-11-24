// kernel/plic.c
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

// 初始化 PLIC (Platform Level Interrupt Controller)
void plic_init(void) {
  // 设置所需的 IRQ 优先级为非零（例如 1）
  // 这样 PLIC 就会将这些中断转发给 CPU
  *(uint32*)(PLIC + UART0_IRQ*4) = 1;
  *(uint32*)(PLIC + VIRTIO0_IRQ*4) = 1;
}

// 为当前 CPU (hart) 初始化中断
void plic_inithart(void) {
  int hart = cpuid();
  
  // 设置当前 hart 的 S-mode 阈值为 0
  // 这样该 hart 就能接收所有优先级 > 0 的中断
  *(uint32*)PLIC_SPRIORITY(hart) = 0;
  
  // 启用 UART 和 VirtIO 的 S-mode 中断
  *(uint32*)PLIC_SENABLE(hart) = (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);
}

// 处理外部中断
// 返回中断号，如果没有中断则返回 0
int plic_claim(void) {
  int hart = cpuid();
  int irq = *(uint32*)PLIC_SCLAIM(hart);
  return irq;
}

// 告知 PLIC 我们已经处理完该中断
void plic_complete(int irq) {
  int hart = cpuid();
  *(uint32*)PLIC_SCLAIM(hart) = irq;
}