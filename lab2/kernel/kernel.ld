/* kernel/kernel.ld */
OUTPUT_ARCH(riscv)
ENTRY(_start)

BASE_ADDRESS = 0x80000000; /* 内核加载的基地址 */

SECTIONS
{
    . = BASE_ADDRESS;

    /* .text 段：存放所有可执行代码 */
    .text : {
        *(.text.entry)
        *(.text .text.*)
    }

    /* .rodata 段：存放只读数据，如字符串常量 */
    .rodata : {
        *(.rodata .rodata.*)
    }

    /* .data 段：存放已初始化的全局变量 */
    .data : {
        *(.data .data.*)
    }

    /* .bss 段：存放未初始化的全局变量，我们的栈也在这里 */
    .bss : {
        /* --- 修改点：使用更直接的语法定义BSS段的起始地址 --- */
        __bss_start = .;
        *(.bss .bss.*)
        /* --- 修改点：使用更直接的语法定义BSS段的结束地址 --- */
        __bss_end = .;
    }

    /* 定义一个符号 end，标记内核的结束地址 */
    PROVIDE(end = .);
}