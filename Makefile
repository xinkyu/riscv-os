# Makefile

# 工具链前缀
TOOLCHAIN = riscv64-unknown-elf-

# 编译器、链接器等
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCHAIN)ld

# 编译和链接标志
CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -mcmodel=medany -ffreestanding -nostdlib -mno-relax
LDFLAGS = -T kernel/kernel.ld

# 需要编译的所有 .c 文件对应的 .o 文件
OBJS = \
    kernel/entry.o \
    kernel/main.o \
    kernel/uart.o

# 默认目标：构建内核
all: kernel.elf

# 链接规则：将所有 .o 文件链接成最终的内核文件
kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJS)

# 汇编文件编译规则：将 .S 文件编译成 .o 文件
kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c -o $@ $<

# C 文件编译规则：将 .c 文件编译成 .o 文件
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 运行规则：使用 QEMU 运行内核
run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel.elf

# 清理规则：删除生成的文件
clean:
	rm -f kernel.elf $(OBJS)

.PHONY: all run clean