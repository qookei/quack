#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include <stdint.h>
#include <stddef.h>

#define GLOBAL_DATA_PTR ((global_data_t *)0xD0000000)

typedef struct {
	uint64_t timer_ticks;
} global_data_t;

#endif
