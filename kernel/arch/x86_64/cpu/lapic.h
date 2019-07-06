#ifndef LAPIC_H
#define LAPIC_H

#include <stdint.h>

void lapic_eoi(void);
void lapic_init(void);

void lapic_write(uint32_t, uint32_t);
uint32_t lapic_read(uint32_t);

void lapic_timer_calc_freq(void);
void lapic_timer_init(void);
void lapic_timer_set_frequency(uint64_t);

void lapic_sleep_ms(uint64_t ms);

#endif
