#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

/* Interrupt handler function pointer */
typedef struct {
    uint64_t rip, cs, rflags, rsp, ss;
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t int_num;
    uint64_t err_code;
} interrupt_frame_t;

void idt_init(void);

/* Register a custom ISR handler (for Phase 2+ driver use) */
typedef void (*isr_handler_t)(interrupt_frame_t *frame);
void idt_register_handler(uint8_t vector, isr_handler_t handler);

#endif /* IDT_H */
