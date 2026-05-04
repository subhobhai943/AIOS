/* ============================================================
 * AIOS Linker Script
 * Places kernel at 1MB physical / 3GB virtual (higher-half)
 * ============================================================ */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(kernel_entry)

SECTIONS
{
    /* Load address: 1 MiB */
    . = 1M;

    .text ALIGN(4K) :
    {
        *(.multiboot2)      /* Multiboot2 header MUST be first */
        *(.text)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata)
    }

    .data ALIGN(4K) :
    {
        *(.data)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
        /* 16 KiB initial kernel stack */
        . = ALIGN(16);
        stack_bottom = .;
        . += 16384;
        stack_top = .;
    }

    kernel_end = .;
}
