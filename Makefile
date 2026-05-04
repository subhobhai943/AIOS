# ============================================================
# AIOS — Build System
# Phase 1: Foundation
# Cross-compiler target: x86_64-elf
# ============================================================

CC      := x86_64-elf-gcc
AS      := nasm
LD      := x86_64-elf-ld

CFLAGS  := -ffreestanding -O2 -Wall -Wextra -fno-exceptions \
           -fno-stack-protector -mno-red-zone -fno-pic \
           -I./kernel/include

LDFLAGS := -T boot/linker.ld -nostdlib

ASFLAGS_BIN  := -f bin
ASFLAGS_ELF  := -f elf64

# Directories
BUILD   := build
ISO     := aios.iso

# Sources
C_SRCS  := $(wildcard kernel/*.c)
ASM_SRCS := $(wildcard kernel/*.asm)

C_OBJS  := $(patsubst kernel/%.c,  $(BUILD)/%.o, $(C_SRCS))
A_OBJS  := $(patsubst kernel/%.asm,$(BUILD)/%_asm.o, $(ASM_SRCS))

.PHONY: all clean iso run

all: $(BUILD)/kernel.bin

# ── Kernel objects ─────────────────────────────────────────
$(BUILD)/%.o: kernel/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%_asm.o: kernel/%.asm | $(BUILD)
	$(AS) $(ASFLAGS_ELF) $< -o $@

# ── Boot object ────────────────────────────────────────────
$(BUILD)/kernel_entry_asm.o: boot/kernel_entry.asm | $(BUILD)
	$(AS) $(ASFLAGS_ELF) $< -o $@

# ── Link ───────────────────────────────────────────────────
$(BUILD)/kernel.bin: $(BUILD)/kernel_entry_asm.o $(C_OBJS) $(A_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# ── ISO (requires GRUB) ────────────────────────────────────
iso: $(BUILD)/kernel.bin
	mkdir -p $(BUILD)/isodir/boot/grub
	cp $(BUILD)/kernel.bin $(BUILD)/isodir/boot/kernel.bin
	cp boot/grub.cfg $(BUILD)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(BUILD)/isodir

# ── Run in QEMU ────────────────────────────────────────────
run: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 512M -serial stdio

# ── Debug in QEMU + GDB ────────────────────────────────────
debug: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 512M -s -S &
	gdb -ex "target remote :1234" -ex "symbol-file $(BUILD)/kernel.bin"

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD) $(ISO)
