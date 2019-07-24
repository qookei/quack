#ifndef CPU_DATA_H
#define CPU_DATA_H

#define DEFAULT_MAX_CPUS 256	// increase this if need be

#include <stdint.h>
#include <stddef.h>

typedef struct {
	int cpu_id;
	uint8_t lapic_id;
	uintptr_t stack_ptr;

	void *gdt;
	void *tss;

	size_t spurious_pic;
} cpu_data_t;


void cpu_data_init(void);
cpu_data_t *cpu_data_get(int);

#endif
