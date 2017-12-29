#include "stacktrace.h"

uint32_t find_correct_trace(uint32_t addr) {
	for (uint32_t i = 0; i < ntrace_elems; ++i) {
		if (addr - trace_elems[i].addr < trace_elems[i].size)
			return i;
	}

	return 0;
}

extern int kprintf(const char*, ...);

void stack_trace(unsigned int max_frames, unsigned int skip) {
	unsigned int *ebp = &max_frames - 2;
    unsigned int skipped = 0;
    kprintf("Stack trace:\n");
    for(unsigned int frame = 0; frame < max_frames; ++frame) {
        unsigned int eip = ebp[1];

        ebp = reinterpret_cast<unsigned int *>(ebp[0]);
        //unsigned int *arguments = &ebp[2];
       	
        if (skipped++ >= skip) {
            int trace_id = find_correct_trace(eip);
            kprintf("0x%x: %s+0x%x\n", eip, trace_elems[trace_id].func_name, eip - trace_elems[trace_id].addr);
        }

        if(ebp == 0)
        	break;

    }
}