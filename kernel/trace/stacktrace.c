#include "stacktrace.h"

void stack_trace(uintptr_t max_frames) {
	uintptr_t *ebp = NULL;
	asm ("mov %%ebp, %0" : "=r"(ebp));
	
	for(unsigned int frame = 0; frame < max_frames; ++frame) {
		uintptr_t eip = ebp[1];
		
		ebp = (uintptr_t *)ebp[0];

		if ((uintptr_t)ebp < 0xC0000000 || (uintptr_t)ebp > 0xCFFFFFFF)
			break;

		kmesg("trace", "#%u: [%08x]", frame, eip);

		if (!ebp)
			break;
    }
}
