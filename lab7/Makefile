# lab7/Makefile

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
    kernel/syscall.o \
    kernel/sysproc.o \
    kernel/spinlock.o \
    kernel/sleeplock.o \
    kernel/bio.o \
    kernel/log.o \
    kernel/fs.o \
    kernel/file.o \
    kernel/sysfile.o

all: kernel.elf

kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJS)

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c -o $@ $<

run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -kernel kernel.elf

clean:
	rm -f kernel.elf $(OBJS)

.PHONY: all run clean