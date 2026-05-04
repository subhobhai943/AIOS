/* ============================================================
 * AIOS — Global Descriptor Table (GDT)
 * 64-bit Long Mode segments
 * Loaded via inline assembly LGDT instruction
 * ============================================================ */

#include "include/gdt.h"
#include <stdint.h>

/* ── GDT entry (64-bit descriptor) ───────────────────────── */
typedef struct __attribute__((packed)) {
    uint16_t limit_low;      /* bits 0-15  of limit */
    uint16_t base_low;       /* bits 0-15  of base  */
    uint8_t  base_mid;       /* bits 16-23 of base  */
    uint8_t  access;         /* access byte         */
    uint8_t  flags_limit_hi; /* flags | bits 16-19 of limit */
    uint8_t  base_high;      /* bits 24-31 of base  */
} gdt_entry_t;

/* ── GDT pointer (for LGDT) ──────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint16_t limit;          /* size - 1  */
    uint64_t base;           /* virtual address of GDT */
} gdt_ptr_t;

/* ── Access byte flags ───────────────────────────────────── */
#define GDT_PRESENT     (1 << 7)
#define GDT_DPL(ring)   ((ring & 3) << 5)
#define GDT_SYSTEM      (1 << 4)  /* 1 = code/data, 0 = system */
#define GDT_EXEC        (1 << 3)
#define GDT_DC          (1 << 2)  /* direction/conforming */
#define GDT_RW          (1 << 1)  /* readable/writable    */
#define GDT_ACCESSED    (1 << 0)

/* ── Flags nibble (high 4 bits of flags_limit_hi byte) ───── */
#define GDT_GRAN_4K     (1 << 7)  /* granularity: 4KB pages  */
#define GDT_64BIT       (1 << 5)  /* 64-bit code segment     */
#define GDT_32BIT       (1 << 6)  /* 32-bit protected mode   */

static gdt_entry_t gdt[GDT_ENTRY_COUNT];
static gdt_ptr_t   gdt_ptr;

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t flags) {
    gdt[idx].base_low       = base & 0xFFFF;
    gdt[idx].base_mid       = (base >> 16) & 0xFF;
    gdt[idx].base_high      = (base >> 24) & 0xFF;
    gdt[idx].limit_low      = limit & 0xFFFF;
    gdt[idx].flags_limit_hi = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt[idx].access         = access;
}

void gdt_init(void) {
    /* 0: Null descriptor */
    gdt_set_entry(GDT_NULL_SEG, 0, 0, 0, 0);

    /* 1: Kernel Code — 64-bit, ring 0 */
    gdt_set_entry(GDT_KERNEL_CODE_SEG, 0, 0xFFFFF,
        GDT_PRESENT | GDT_DPL(0) | GDT_SYSTEM | GDT_EXEC | GDT_RW,
        GDT_GRAN_4K | GDT_64BIT);

    /* 2: Kernel Data — ring 0 */
    gdt_set_entry(GDT_KERNEL_DATA_SEG, 0, 0xFFFFF,
        GDT_PRESENT | GDT_DPL(0) | GDT_SYSTEM | GDT_RW,
        GDT_GRAN_4K | GDT_32BIT);

    /* 3: User Code — 64-bit, ring 3 (Phase 4) */
    gdt_set_entry(GDT_USER_CODE_SEG, 0, 0xFFFFF,
        GDT_PRESENT | GDT_DPL(3) | GDT_SYSTEM | GDT_EXEC | GDT_RW,
        GDT_GRAN_4K | GDT_64BIT);

    /* 4: User Data — ring 3 (Phase 4) */
    gdt_set_entry(GDT_USER_DATA_SEG, 0, 0xFFFFF,
        GDT_PRESENT | GDT_DPL(3) | GDT_SYSTEM | GDT_RW,
        GDT_GRAN_4K | GDT_32BIT);

    /* 5: TSS placeholder (Phase 4) */
    gdt_set_entry(GDT_TSS_SEG, 0, 0, 0, 0);

    /* Load GDTR */
    gdt_ptr.limit = (uint16_t)(sizeof(gdt) - 1);
    gdt_ptr.base  = (uint64_t)&gdt;

    __asm__ volatile (
        "lgdt %0\n"
        "mov %1, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        /* Far return to reload CS with new kernel code selector */
        "push %2\n"
        "lea  1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        :
        : "m"(gdt_ptr), "i"(GDT_KERNEL_DS), "i"(GDT_KERNEL_CS)
        : "rax", "memory"
    );
}
