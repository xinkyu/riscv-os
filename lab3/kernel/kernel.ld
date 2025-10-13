/*
 * kernel/kernel.ld (修正版，与 entry.S 保持一致)
 */
OUTPUT_ARCH(riscv)
ENTRY(_start) /* 关键修改1: 将入口点从 _entry 改为 _start */

BASE_ADDRESS = 0x80000000;

SECTIONS
{
    . = BASE_ADDRESS;

    /* 代码段 .text */
    .text : {
        *(.text.entry)
        *(.text .text.*)
        . = ALIGN(4K);
        etext = .; /* 定义 etext 供 vm.c 使用 */
    }

    /* 只读数据段 .rodata */
    .rodata : {
        *(.rodata .rodata.*)
    }

    /* 已初始化数据段 .data */
    .data : {
        *(.data .data.*)
    }

    /* .bss 段: 包含所有未初始化数据和栈 */
    .bss : {
        /* 关键修改2: 定义 entry.S 需要的 BSS 符号 */
        __bss_start = .; 
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* 定义内核镜像的结束地址 */
    PROVIDE(end = .);
}