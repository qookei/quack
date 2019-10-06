#include "cpu_data.h"
#include <mm/heap.h>
#include <panic.h>
#include <kmesg.h>
#include <acpi/acpi.h>
#include <stdlib.h>
#include <cmdline.h>
#include <mm/pmm.h>
#include <mm/mm.h>

static cpu_data_t *cpu_data = NULL;
static int max_cpus;

// cpu argument format
// cpu=<MAX CORES>

void cpu_data_init(void) {
	char **cpu_args;
	if (!(cpu_args = cmdline_get_values("cpu")))
		max_cpus = DEFAULT_MAX_CPUS;
	else
		max_cpus = atoi(cpu_args[0]);

	cpu_data = (cpu_data_t *)(VIRT_PHYS_BASE + (uintptr_t)pmm_alloc(((max_cpus * sizeof(cpu_data_t)) + 0xFFF / 0x1000)));
}

cpu_data_t *cpu_data_get(int cpu) {
	if (cpu >= max_cpus)
		return NULL;

	return &cpu_data[cpu];
}
