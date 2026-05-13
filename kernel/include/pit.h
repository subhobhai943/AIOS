#ifndef PIT_H
#define PIT_H

#include <stdint.h>

#define PIT_BASE_FREQ 1193180
#define PIT_HZ 100

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43

void pit_init(uint32_t hz);

void pit_handler(void);

uint64_t pit_get_ticks(void);

uint64_t pit_get_ms(void);

void pit_sleep_ms(uint64_t ms);

#endif
