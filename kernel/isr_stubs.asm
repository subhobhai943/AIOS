; ============================================================
; AIOS — ISR Stubs (Assembly)
; 256 stubs that push vector number + call isr_dispatch()
; NASM, ELF64
; ============================================================

bits 64
extern isr_dispatch

; ── Common ISR handler ────────────────────────────────────
isr_common_handler:
    ; Push general-purpose registers (in interrupt_frame_t order)
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    ; rdi = pointer to interrupt_frame_t (first arg in SysV ABI)
    mov  rdi, rsp
    call isr_dispatch

    ; Restore registers
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; Skip int_num + err_code pushed by stub
    add rsp, 16
    iretq

; ── Macro to generate stubs ───────────────────────────────
; Some exceptions push an error code; most do not.
; When no error code: push dummy 0, then vector number.
; When error code already on stack: push vector number only.

%macro ISR_NO_ERR 1
global isr_stub_%1
isr_stub_%1:
    push qword 0        ; dummy error code
    push qword %1       ; interrupt vector
    jmp  isr_common_handler
%endmacro

%macro ISR_ERR 1
global isr_stub_%1
isr_stub_%1:
    push qword %1       ; interrupt vector (error code already pushed by CPU)
    jmp  isr_common_handler
%endmacro

; ── CPU Exceptions (0x00-0x1F) ────────────────────────────
ISR_NO_ERR 0    ; Divide Error
ISR_NO_ERR 1    ; Debug
ISR_NO_ERR 2    ; NMI
ISR_NO_ERR 3    ; Breakpoint
ISR_NO_ERR 4    ; Overflow
ISR_NO_ERR 5    ; Bound Range Exceeded
ISR_NO_ERR 6    ; Invalid Opcode
ISR_NO_ERR 7    ; Device Not Available
ISR_ERR    8    ; Double Fault        (error code = 0)
ISR_NO_ERR 9    ; Coprocessor Overrun
ISR_ERR    10   ; Invalid TSS
ISR_ERR    11   ; Segment Not Present
ISR_ERR    12   ; Stack-Segment Fault
ISR_ERR    13   ; General Protection
ISR_ERR    14   ; Page Fault
ISR_NO_ERR 15   ; Reserved
ISR_NO_ERR 16   ; x87 FPU
ISR_ERR    17   ; Alignment Check
ISR_NO_ERR 18   ; Machine Check
ISR_NO_ERR 19   ; SIMD FP
ISR_NO_ERR 20   ; Virtualization
ISR_ERR    21   ; Control Protection
ISR_NO_ERR 22
ISR_NO_ERR 23
ISR_NO_ERR 24
ISR_NO_ERR 25
ISR_NO_ERR 26
ISR_NO_ERR 27
ISR_NO_ERR 28   ; Hypervisor Injection
ISR_ERR    29   ; VMM Communication
ISR_ERR    30   ; Security
ISR_NO_ERR 31

; ── Hardware IRQs (0x20-0x2F) ─────────────────────────────
%assign i 32
%rep 16
ISR_NO_ERR i
%assign i i+1
%endrep

; ── Remaining vectors (0x30-0xFF) ─────────────────────────
%assign i 48
%rep 208
ISR_NO_ERR i
%assign i i+1
%endrep

; ── Stub pointer table (exported for idt.c) ───────────────
section .data
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    dq isr_stub_%+i
%assign i i+1
%endrep
