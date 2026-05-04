#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* GDT segment indices */
#define GDT_NULL_SEG        0
#define GDT_KERNEL_CODE_SEG 1
#define GDT_KERNEL_DATA_SEG 2
#define GDT_USER_CODE_SEG   3
#define GDT_USER_DATA_SEG   4
#define GDT_TSS_SEG         5
#define GDT_ENTRY_COUNT     6

/* Segment selectors (index << 3 | RPL) */
#define GDT_KERNEL_CS  (GDT_KERNEL_CODE_SEG << 3)
#define GDT_KERNEL_DS  (GDT_KERNEL_DATA_SEG << 3)
#define GDT_USER_CS    ((GDT_USER_CODE_SEG   << 3) | 3)
#define GDT_USER_DS    ((GDT_USER_DATA_SEG   << 3) | 3)

void gdt_init(void);

#endif /* GDT_H */
