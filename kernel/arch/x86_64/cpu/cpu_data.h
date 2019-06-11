#ifndef CPU_DATA_H
#define CPU_DATA_H

#define MAX_CPUS 256

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint8_t lapic_id;
	uintptr_t stack_ptr;
	int enabled;

	size_t spurious_pic;
} cpu_data_t;

void cpu_data_init(void);
cpu_data_t *cpu_data_get_for_cpu(int);

#endif
