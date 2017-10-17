#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>
#include <stddef.h>

typedef struct {

	const char *func_name;
	uint32_t addr;
	uint32_t size;

} trace_elem;

extern "C" {
	extern trace_elem trace_elems[];
	extern uint32_t ntrace_elems;
}

#endif