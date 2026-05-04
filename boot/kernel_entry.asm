; ============================================================
; AIOS — Kernel Entry Point
; Multiboot2 header + 64-bit Long Mode entry
; NASM syntax, ELF64 output
; ============================================================

bits 32

; ── Multiboot2 constants ────────────────────────────────────
MB2_MAGIC       equ 0xE85250D6
MB2_ARCH_X86    equ 0
MB2_HEADER_LEN  equ (mb2_header_end - mb2_header_start)
MB2_CHECKSUM    equ -(MB2_MAGIC + MB2_ARCH_X86 + MB2_HEADER_LEN)

section .multiboot2
align 8
mb2_header_start:
    dd MB2_MAGIC
    dd MB2_ARCH_X86
    dd MB2_HEADER_LEN
    dd MB2_CHECKSUM
    ; End tag
    dw 0                ; type
    dw 0                ; flags
    dd 8                ; size
mb2_header_end:

; ── Entry point ─────────────────────────────────────────────
section .text
global kernel_entry
extern kernel_main
extern gdt_flush
extern idt_flush

kernel_entry:
    ; Set up stack (defined in linker script)
    mov esp, stack_top

    ; Save Multiboot2 magic and info pointer
    push ebx            ; Multiboot2 info struct pointer
    push eax            ; Multiboot2 magic value

    ; Call C kernel
    call kernel_main

    ; If kernel_main returns, halt forever
.halt:
    cli
    hlt
    jmp .halt

; ── Stack (referenced by linker script) ─────────────────────
section .bss
align 16
global stack_bottom
global stack_top
stack_bottom:
    resb 16384          ; 16 KiB kernel stack
stack_top:
