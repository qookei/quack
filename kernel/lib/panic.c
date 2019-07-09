#include "panic.h"
#include <arch/cpu.h>
#include <kmesg.h>
#include <stdarg.h>
#include <string.h>
#include <vsnprintf.h>
#include <spinlock.h>

void panic(void *state, const char *message, ...) {
	char buf[128 + 16];
	va_list va;
	memset(buf, 0, 128 + 16);

	va_start(va, message);
	vsnprintf(buf, 128, message, va);
	va_end(va);

	kmesg("kernel", "Kernel panic!");
	kmesg("kernel", "Message: '%s'", buf);

	if (state)
		arch_cpu_dump_state(state);

	arch_cpu_trace_stack();

	kmesg("kernel", "halting");

	arch_cpu_halt_forever();
}
