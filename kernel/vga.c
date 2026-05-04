/* ============================================================
 * AIOS — VGA Text Mode Driver
 * Writes directly to the VGA text buffer at 0xB8000
 * 80x25, 16 colors, no BIOS dependency
 * ============================================================ */

#include "include/vga.h"
#include <stdint.h>
#include <stddef.h>

/* I/O port helpers (no stdlib) */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static volatile uint16_t *const VGA_MEM = (volatile uint16_t*)0xB8000;

/* Driver state */
static size_t   vga_col   = 0;
static size_t   vga_row   = 0;
static uint8_t  vga_attr  = 0;  /* fg | (bg << 4) */

/* ── Helpers ──────────────────────────────────────────────── */
static inline uint8_t make_attr(vga_color_t fg, vga_color_t bg) {
    return (uint8_t)(fg | (bg << 4));
}

static inline uint16_t make_entry(char c, uint8_t attr) {
    return (uint16_t)(uint8_t)c | ((uint16_t)attr << 8);
}

/* Update hardware cursor via VGA CRTC registers */
static void hw_cursor_update(void) {
    uint16_t pos = (uint16_t)(vga_row * VGA_WIDTH + vga_col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/* Scroll the screen up by one row */
static void vga_scroll(void) {
    /* Move rows 1..24 up by one */
    for (size_t row = 1; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            VGA_MEM[(row - 1) * VGA_WIDTH + col] =
                VGA_MEM[row * VGA_WIDTH + col];
        }
    }
    /* Clear last row */
    for (size_t col = 0; col < VGA_WIDTH; col++) {
        VGA_MEM[(VGA_HEIGHT - 1) * VGA_WIDTH + col] =
            make_entry(' ', vga_attr);
    }
    vga_row = VGA_HEIGHT - 1;
}

/* ── Public API ───────────────────────────────────────────── */
void vga_init(void) {
    vga_attr = make_attr(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_attr = make_attr(fg, bg);
}

void vga_clear(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEM[i] = make_entry(' ', vga_attr);
    }
    vga_col = 0;
    vga_row = 0;
    hw_cursor_update();
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        /* 4-space tab */
        size_t next = (vga_col + 4) & ~3u;
        while (vga_col < next) vga_putchar(' ');
        return;
    } else {
        VGA_MEM[vga_row * VGA_WIDTH + vga_col] = make_entry(c, vga_attr);
        vga_col++;
    }

    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }
    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
    }
    hw_cursor_update();
}

void vga_puts(const char *str) {
    while (*str) vga_putchar(*str++);
}

void vga_puts_color(const char *str, vga_color_t fg, vga_color_t bg) {
    uint8_t saved = vga_attr;
    vga_set_color(fg, bg);
    vga_puts(str);
    vga_attr = saved;
}

void vga_puthex(uint64_t value) {
    const char *hex = "0123456789ABCDEF";
    vga_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        vga_putchar(hex[(value >> i) & 0xF]);
    }
}

void vga_putdec(uint64_t value) {
    if (value == 0) { vga_putchar('0'); return; }
    char buf[21];
    int  idx = 0;
    while (value > 0) {
        buf[idx++] = (char)('0' + (value % 10));
        value /= 10;
    }
    for (int i = idx - 1; i >= 0; i--) vga_putchar(buf[i]);
}

void vga_set_cursor(size_t col, size_t row) {
    vga_col = col;
    vga_row = row;
    hw_cursor_update();
}
