# Makefile for lab6 kernel

TOOLCHAIN = riscv64-unknown-elf-
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCHAIN)ld

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb \
         -mcmodel=medany -ffreestanding -nostdlib -mno-relax -Ikernel/
LDFLAGS = -T kernel/kernel.ld

OBJS = \
	kernel/entry.o \
	kernel/main.o \
	kernel/uart.o \
	kernel/console.o \
	kernel/printf.o \
	kernel/kalloc.o \
	kernel/vm.o \
	kernel/trap.o \
	kernel/kernelvec.o \
	kernel/proc.o \
	kernel/swtch.o \
	kernel/syscall.o \
	kernel/sysproc.o

# 默认目标
all: kernel.elf

# 链接所有目标文件生成内核
kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# 编译 C 源文件为 .o
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 编译汇编源文件为 .o
kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c -o $@ $<

# 启动 QEMU 运行内核
run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -kernel kernel.elf

# 清理编译产物
clean:
	rm -f kernel.elf $(OBJS)

.PHONY: all run clean
