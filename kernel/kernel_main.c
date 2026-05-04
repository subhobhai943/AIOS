/* ============================================================
 * AIOS — Kernel Main
 * Entry point called from boot/kernel_entry.asm
 * Phase 1: Foundation
 *
 * Roadmap exit criteria:
 *   ✅ OS boots from .iso
 *   ✅ Switches to 64-bit Long Mode (via GRUB Multiboot2)
 *   ✅ Prints "AIOS Initialized" to VGA screen
 *   ✅ GDT loaded with kernel code/data segments
 *   ✅ IDT loaded, PIC remapped, exceptions handled
 *   ✅ System does not crash
 * ============================================================ */

#include "include/vga.h"
#include "include/gdt.h"
#include "include/idt.h"
#include <stdint.h>

/* Multiboot2 magic value passed in EAX by GRUB */
#define MULTIBOOT2_MAGIC 0x36D76289

/* ── Simple string helper (no stdlib) ──────────────────── */
static void print_banner(void) {
    vga_puts_color(
        "======================================================\n",
        VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    vga_puts_color(
        "   AIOS — Autonomous Intelligent Operating System     \n",
        VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts_color(
        "   Phase 1: Foundation                               \n",
        VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts_color(
        "======================================================\n",
        VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    vga_puts("\n");
}

static void print_ok(const char *label) {
    vga_puts_color("  [ ", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts_color("OK",   VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts_color(" ] ",  VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts(label);
    vga_puts("\n");
}

/* ── Kernel entry ──────────────────────────────────────── */
void kernel_main(uint32_t mb2_magic, uint32_t /*mb2_info_ptr*/) {
    /* 1. Init VGA text driver (clears screen) */
    vga_init();
    print_banner();

    /* 2. Verify Multiboot2 handoff */
    if (mb2_magic == MULTIBOOT2_MAGIC) {
        print_ok("Multiboot2 handoff verified");
    } else {
        vga_puts_color("  [WARN] Not booted via Multiboot2 — some features may differ\n",
                       VGA_COLOR_BROWN, VGA_COLOR_BLACK);
    }

    /* 3. Load GDT */
    gdt_init();
    print_ok("GDT loaded (null / kernel-code / kernel-data / user-code / user-data)");

    /* 4. Load IDT + remap PIC */
    idt_init();
    print_ok("IDT loaded, PIC remapped (IRQ0-15 → INT 0x20-0x2F)");
    print_ok("STI — interrupts enabled");

    /* ── Phase 1 exit criteria met ───────────────────────── */
    vga_puts("\n");
    vga_puts_color(
        "  AIOS Initialized\n",
        VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("\n");
    vga_puts_color(
        "  Roadmap Phase 1 complete. Ready for Phase 2 (Memory Management).\n",
        VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("\n");

    /* ── Idle loop ────────────────────────────────────────── */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
