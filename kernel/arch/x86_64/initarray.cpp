#include <stdint.h>
#include <stddef.h>

#include <kmesg.h>

extern "C" {
	extern void *_init_array_begin;
	extern void *_init_array_end;
}

void initarray_run() {
	uintptr_t start = (uintptr_t)&_init_array_begin;
	uintptr_t end = (uintptr_t)&_init_array_end;

	for (uintptr_t ptr = start; ptr < end; ptr += 8) {
		auto fn = (void(*)())(*(uintptr_t *)ptr);
		fn();
	}
}

