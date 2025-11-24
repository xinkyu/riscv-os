# Makefile
TOOLCHAIN = riscv64-unknown-elf-
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCHAIN)ld

BSIZE = 1024
FSSIZE = 4096

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -mcmodel=medany -ffreestanding -nostdlib -mno-relax -Ikernel/
LDFLAGS = -T kernel/kernel.ld

FSIMG = fs.img

OBJS = \
    kernel/entry.o        \
    kernel/main.o         \
    kernel/uart.o         \
    kernel/console.o      \
    kernel/printf.o       \
    kernel/kalloc.o       \
    kernel/vm.o           \
    kernel/trap.o         \
    kernel/kernelvec.o    \
    kernel/proc.o         \
    kernel/swtch.o        \
    kernel/spinlock.o     \
    kernel/sleeplock.o   \
    kernel/string.o      \
    kernel/syscall.o      \
    kernel/sysproc.o      \
    kernel/sysfile.o      \
    kernel/bio.o         \
    kernel/log.o         \
    kernel/fs.o          \
    kernel/file.o        \
    kernel/virtio_disk.o \
    kernel/test.o

all: kernel.elf

kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJS)

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c -o $@ $<

run: kernel.elf $(FSIMG)
	qemu-system-riscv64 -machine virt -nographic -kernel kernel.elf \
		-drive file=$(FSIMG),if=none,format=raw,id=fsimg \
		-device virtio-blk-device,drive=fsimg,bus=virtio-mmio-bus.0

$(FSIMG):
	dd if=/dev/zero of=$(FSIMG) bs=$(BSIZE) count=$(FSSIZE)

clean:
	rm -f kernel.elf $(OBJS) $(FSIMG)

.PHONY: all run clean