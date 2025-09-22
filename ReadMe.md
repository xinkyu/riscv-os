# 从零构建操作系统 - RISC-V 专案

本专案是武汉大学《操作系统实践A》课程的成果。
目标是从零开始，基于 RISC-V 架构，逐步实现一个迷你的操作系统核心。

## 目前完成进度

* **实验 1**: RISC-V 引导与裸机启动
    * 实现了基本的汇编语言启动程序。
    * 建立了 C 语言执行环境并设置了堆栈。
    * 实现了简化的 UART 驱动，可在 QEMU 中输出 "Hello OS"。
* **实验 2**: 内核 printf 与清屏功能实现
    * 实现了支持 `%d`、`%x`、`%s`、`%c`、`%%` 的 `printf` 函数。
    * 实现了基于 ANSI 转义序列的清屏功能。

## 开发环境

* [cite_start]**操作系统**: Ubuntu 22.04 LTS [cite: 2541]
* [cite_start]**工具链**: riscv64-unknown-elf-gcc [cite: 2990]
* [cite_start]**模拟器**: QEMU (qemu-system-riscv64) [cite: 2989]

## 编译与执行

1.  **编译内核**：
    ```bash
    make
    ```

2.  **在 QEMU 中执行**：
    ```bash
    make run
    ```