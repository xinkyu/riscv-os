# kernel/entry.S

# .section 指令告诉汇编器将接下来的代码放入 .text.entry 段
.section .text.entry
# .global 指令使得 _start 符号对链接器可见，作为程序的入口点
.global _start

_start:
    # 设置栈指针 (sp)。操作系统需要一个栈来保存函数调用的返回地址、局部变量等。
    # 我们为栈预留了 4KB 空间，stack_top 是这段空间的最高地址
    la sp, stack_top

    # 清零 BSS 段 
    # 加载 BSS 段的起始和结束地址
    la a0, __bss_start
    la a1, __bss_end

clear_bss_loop:
    # 比较 a0 和 a1，如果 a0 >= a1，说明 BSS 段已全部清零，跳转到 clear_bss_done
    bge a0, a1, clear_bss_done
    # 将 0 存入 a0 指向的内存地址（8字节/64位）
    # RISC-V中，sd (store double word) 指令用于写入8字节
    sd zero, 0(a0)
    # 将 a0 的地址增加 8，指向下一个8字节
    addi a0, a0, 8
    # 无条件跳转回循环开始处
    j clear_bss_loop

clear_bss_done:

    # 调用C语言的主函数 kmain
    call kmain

# 如果 kmain 意外返回，程序会进入一个无限循环，防止 CPU "跑飞"。
halt:
    j halt

# 在 .bss 段中为我们的栈保留 4096 字节的空间。该段用于存放未初始化的全局变量和静态变量
.section .bss
.align 16           # 16字节对齐
stack_bottom:
.skip 4096          # 预留 4096 字节
stack_top:          # 栈顶地址的标签