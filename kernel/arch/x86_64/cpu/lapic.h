#ifndef LAPIC_H
#define LAPIC_H

#include <stdint.h>

void lapic_eoi(void);
void lapic_init(void);

void lapic_timer_calc_freq(void);
void lapic_timer_init(void);
void lapic_timer_set_frequency(uint64_t);

#endif
