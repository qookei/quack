#include "cpu_data.h"
#include <mm/heap.h>
#include <panic.h>
#include <kmesg.h>
#include <acpi/acpi.h>

static cpu_data_t *cpu_data = NULL;
static int cpu_data_n_cpus = 0;

void cpu_data_init(void) {
	int n_cpus = madt_get_lapic_count();

	if (n_cpus > MAX_CPUS)
		panic(NULL, "Number of max cpus (%d) exceeded", MAX_CPUS);

	cpu_data = kcalloc(n_cpus, sizeof(cpu_data_t));
	cpu_data_n_cpus = n_cpus;

	madt_lapic_t *lapics = madt_get_lapics();
	for (int i = 0; i < n_cpus; i++) {
		cpu_data[i].lapic_id = lapics[i].apic_id;
		cpu_data[i].enabled = lapics[i].flags & 1;
	}

	kmesg("cpu", "per-cpu data initialized");
}

cpu_data_t *cpu_data_get_for_cpu(int cpu) {
	if (cpu >= cpu_data_n_cpus)
		return NULL;

	return &cpu_data[cpu];
}
