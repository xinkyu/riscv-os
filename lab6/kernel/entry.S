# kernel/entry.S

.section .text.entry
.global _start

_start:
    # 移除 M-mode 的中断委托代码。
    # 因为 QEMU 的默认固件 (OpenSBI) 会为我们完成这项工作，
    # 并且已经将 CPU 置于 S-mode 来执行这里的代码。

    # 设置栈指针 (sp)。
    la sp, stack_top

    # 清零 BSS 段 
    la a0, __bss_start
    la a1, __bss_end

clear_bss_loop:
    bge a0, a1, clear_bss_done
    sd zero, 0(a0)
    addi a0, a0, 8
    j clear_bss_loop

clear_bss_done:

    # 调用C语言的主函数 kmain
    call kmain

halt:
    j halt

.section .bss
.align 16
stack_bottom:
.skip 4096
stack_top: