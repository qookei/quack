#ifndef LAPIC_H
#define LAPIC_H

#include <stdint.h>

void lapic_eoi(void);
void lapic_init(void);

void lapic_write(uint32_t, uint32_t);
uint32_t lapic_read(uint32_t);

void lapic_timer_calc_freq_bsp(void);
void lapic_timer_init_bsp(void);
void lapic_timer_init_ap(void);

void lapic_sleep_ms_bsp(uint64_t ms);

#endif
