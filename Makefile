# Makefile for the minimal RISC-V OS

# 1. 工具链定义
TOOLCHAIN = riscv64-unknown-elf-
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCHAIN)ld

# 2. 编译标志
# -Wall -Werror: 开启所有警告并视作错误
# -O: 优化
# -fno-omit-frame-pointer: 保留栈帧指针，方便调试
# -ggdb: 生成GDB调试信息
# -mcmodel=medany: 中等代码模型
# -ffreestanding: 独立环境，不依赖标准库
# -nostdlib: 不链接标准库
# -mno-relax: 关闭链接器松弛优化
# -Ikernel/: 指定头文件搜索路径为 kernel/ 目录 (关键修复点)
CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -mcmodel=medany -ffreestanding -nostdlib -mno-relax -Ikernel/

# 3. 链接标志
# -T kernel/kernel.ld: 使用我们自定义的链接器脚本
LDFLAGS = -T kernel/kernel.ld

# 4. 需要编译的目标文件列表
OBJS = \
    kernel/entry.o \
    kernel/main.o \
    kernel/uart.o \
    kernel/printf.o

# 5. 默认构建目标
# 当直接运行 make 时，会执行这个规则
all: kernel.elf

# 6. 链接规则
# 将所有的 .o 文件链接成最终的 kernel.elf
kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJS)

# 7. C 文件编译规则
# 这是一个模式规则，用于将任何 kernel/ 目录下的 .c 文件编译成对应的 .o 文件
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 8. 汇编文件编译规则
# 用于将 kernel/ 目录下的 .S 文件编译成对应的 .o 文件
kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c -o $@ $<

# 9. 运行规则
run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel.elf

# 10. 清理规则
clean:
	rm -f kernel.elf $(OBJS)

# 11. 伪目标
# 声明 all, run, clean 不是真正的文件名
.PHONY: all run clean