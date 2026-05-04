/* ============================================================
 * AIOS — Interrupt Descriptor Table (IDT)
 * 256 gate descriptors, covering:
 *   0x00-0x1F  CPU exceptions
 *   0x20-0x2F  PIC hardware IRQs (after remapping)
 *   0x80       Future syscall gate
 * ============================================================ */

#include "include/idt.h"
#include "include/vga.h"
#include <stdint.h>

/* ── IDT Gate Descriptor (16 bytes, 64-bit Interrupt Gate) ── */
typedef struct __attribute__((packed)) {
    uint16_t offset_low;    /* ISR address bits 0-15   */
    uint16_t selector;      /* Code segment selector   */
    uint8_t  ist;           /* Interrupt Stack Table   */
    uint8_t  type_attr;     /* Gate type + DPL + present */
    uint16_t offset_mid;    /* ISR address bits 16-31  */
    uint32_t offset_high;   /* ISR address bits 32-63  */
    uint32_t zero;          /* Reserved                */
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} idt_ptr_t;

/* ── Gate type constants ─────────────────────────────────── */
#define IDT_INTERRUPT_GATE  0x8E  /* Present, ring0, 64-bit interrupt */
#define IDT_TRAP_GATE       0x8F  /* Present, ring0, 64-bit trap      */

/* ── Tables ─────────────────────────────────────────────── */
static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;

/* Custom handler table (populated via idt_register_handler) */
static isr_handler_t isr_handlers[IDT_ENTRIES] = {0};

/* ── ISR stubs declared in kernel/isr_stubs.asm ─────────── */
extern void *isr_stub_table[IDT_ENTRIES];

/* ── I/O port helpers ─────────────────────────────────────  */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* ── PIC remap ────────────────────────────────────────────  */
#define PIC1_CMD   0x20
#define PIC1_DATA  0x21
#define PIC2_CMD   0xA0
#define PIC2_DATA  0xA1
#define PIC_EOI    0x20

static void pic_remap(int offset1, int offset2) {
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    outb(PIC1_CMD,  0x11);  /* ICW1: init + ICW4 */
    outb(PIC2_CMD,  0x11);
    outb(PIC1_DATA, (uint8_t)offset1);  /* ICW2: vector offsets */
    outb(PIC2_DATA, (uint8_t)offset2);
    outb(PIC1_DATA, 0x04);  /* ICW3: cascade identity */
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);  /* ICW4: 8086 mode */
    outb(PIC2_DATA, 0x01);

    /* Restore saved masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/* ── IDT gate writer ──────────────────────────────────────  */
static void idt_set_gate(uint8_t vector, void *isr_addr,
                         uint8_t type_attr) {
    uint64_t addr = (uint64_t)isr_addr;
    idt[vector].offset_low  = addr & 0xFFFF;
    idt[vector].selector    = 0x08;          /* kernel code selector */
    idt[vector].ist         = 0;
    idt[vector].type_attr   = type_attr;
    idt[vector].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

/* ── Public C interrupt dispatcher ──────────────────────── */
void isr_dispatch(interrupt_frame_t *frame) {
    uint64_t vec = frame->int_num;

    if (isr_handlers[vec]) {
        isr_handlers[vec](frame);
    } else if (vec < 32) {
        /* Unhandled CPU exception — print and halt */
        static const char *exc_names[] = {
            "Division Error",       "Debug",                "NMI",
            "Breakpoint",           "Overflow",             "Bound Range",
            "Invalid Opcode",       "Device Not Available", "Double Fault",
            "Coprocessor Overrun",  "Invalid TSS",          "Segment Not Present",
            "Stack Fault",          "General Protection",   "Page Fault",
            "Reserved",             "x87 FP",               "Alignment Check",
            "Machine Check",        "SIMD FP",              "Virtualization",
            "Control Protection",   "Reserved",             "Reserved",
            "Reserved",             "Reserved",             "Reserved",
            "Reserved",             "Hypervisor",           "VMM Communication",
            "Security",             "Reserved"
        };
        vga_puts_color("\n[AIOS PANIC] CPU Exception #",
                       VGA_COLOR_WHITE, VGA_COLOR_RED);
        vga_putdec(vec);
        vga_puts_color(" — ", VGA_COLOR_WHITE, VGA_COLOR_RED);
        vga_puts_color(exc_names[vec], VGA_COLOR_WHITE, VGA_COLOR_RED);
        vga_puts_color("\nError Code: ", VGA_COLOR_WHITE, VGA_COLOR_RED);
        vga_puthex(frame->err_code);
        vga_puts_color("\nRIP: ", VGA_COLOR_WHITE, VGA_COLOR_RED);
        vga_puthex(frame->rip);
        vga_puts_color("\nSystem Halted.\n", VGA_COLOR_WHITE, VGA_COLOR_RED);
        __asm__ volatile ("cli; hlt");
    }

    /* Send EOI to PIC for hardware IRQs */
    if (vec >= 32 && vec < 48) {
        if (vec >= 40) outb(PIC2_CMD, PIC_EOI);
        outb(PIC1_CMD, PIC_EOI);
    }
}

void idt_register_handler(uint8_t vector, isr_handler_t handler) {
    isr_handlers[vector] = handler;
}

void idt_init(void) {
    /* Remap PIC: IRQ0-7 → INT 0x20-0x27, IRQ8-15 → INT 0x28-0x2F */
    pic_remap(0x20, 0x28);

    /* Fill IDT from the stub table generated in isr_stubs.asm */
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate((uint8_t)i, isr_stub_table[i], IDT_INTERRUPT_GATE);
    }

    idt_ptr.limit = (uint16_t)(sizeof(idt) - 1);
    idt_ptr.base  = (uint64_t)&idt;

    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
    __asm__ volatile ("sti");
}
