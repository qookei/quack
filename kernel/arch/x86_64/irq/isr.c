
#include <io/debug.h>

void dispatch_interrupt(void *a) {
	debug_putstr("quack: interrupt fired\n");
}

