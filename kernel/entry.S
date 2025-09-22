# kernel/entry.S

# .section 指令告诉汇编器将接下来的代码放入 .text.entry 段
.section .text.entry
# .global 指令使得 _start 符号对链接器可见，作为程序的入口点
.global _start

_start:
    # 设置栈指针 (sp)。我们为栈预留了 4KB 空间。
    #  将stack_top 的地址加载到 sp 寄存器中。
    la sp, stack_top

    # 调用C语言的主函数 kmain
    call kmain

# 如果 kmain 意外返回，程序会进入一个无限循环，防止 CPU "跑飞"。
halt:
    j halt

# 在 .bss 段中为我们的栈保留 4096 字节的空间。

.section .bss
.align 16           # 16字节对齐
stack_bottom:
.skip 4096          # 预留 4096 字节
stack_top:          # 栈顶地址的标签