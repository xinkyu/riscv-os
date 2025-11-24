# Makefile
TOOLCHAIN = riscv64-unknown-elf-
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCHAIN)ld

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -mcmodel=medany -ffreestanding -nostdlib -mno-relax -Ikernel/
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
    kernel/spinlock.o \
    kernel/syscall.o \
    kernel/sysproc.o \
    kernel/sysfile.o \
    kernel/test.o \
    kernel/plic.o \
    kernel/virtio_disk.o \
    kernel/bio.o \
    kernel/log.o \
    kernel/fs.o \
    kernel/file.o \
    kernel/string.o  # 如果没有 string.c，需要在 defs.h 内联实现 memset/memmove 或在某个 .c 文件中实现

# 确保有 fs.img 
# 通常这里需要 mkfs，为简单起见，我们让 QEMU 挂载一个空文件或预制文件
# 或者你可以添加规则：
fs.img:
	dd if=/dev/zero of=fs.img bs=1M count=32

run: kernel.elf fs.img
	qemu-system-riscv64 -machine virt -nographic -kernel kernel.elf \
	-drive file=fs.img,if=none,format=raw,id=x0 \
	-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
clean:
	rm -f kernel.elf $(OBJS)

.PHONY: all run clean