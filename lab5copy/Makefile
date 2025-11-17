# lab6/Makefile

TOOLCHAIN = riscv64-unknown-elf-
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCHAIN)ld
OBJCOPY = $(TOOLCHAIN)objcopy

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -mcmodel=medany \
         -ffreestanding -fno-common -nostdlib -mno-relax -Ikernel/
LDFLAGS = -T kernel/kernel.ld

KERN_OBJS = \
    kernel/entry.o \
    kernel/main.o \
    kernel/uart.o \
    kernel/console.o \
    kernel/printf.o \
    kernel/kalloc.o \
    kernel/vm.o \
    kernel/trap.o \
    kernel/kernelvec.o \
    kernel/trampoline.o \
    kernel/test.o \
    kernel/proc.o \
    kernel/swtch.o \
    kernel/spinlock.o \
    kernel/syscall.o \
    kernel/sysproc.o \
    kernel/sysfile.o \
    kernel/string.o

USER_PROGS = user/_initcode

all: kernel.elf

kernel.elf: $(KERN_OBJS) initcode
	$(LD) $(LDFLAGS) -o kernel.elf $(KERN_OBJS)

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c -o $@ $

kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c -o $@ $

# 构建 initcode
initcode: user/initcode.S
	$(CC) $(CFLAGS) -c user/initcode.S -o user/initcode.o
	$(LD) -N -e start -Ttext 0 -o user/initcode.out user/initcode.o
	$(OBJCOPY) -S -O binary user/initcode.out user/initcode
	$(OBJCOPY) -I binary -O elf64-littleriscv -B riscv \
		user/initcode kernel/initcode.o

run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -kernel kernel.elf

clean:
	rm -f kernel.elf $(KERN_OBJS) user/*.o user/initcode user/initcode.out

.PHONY: all run clean