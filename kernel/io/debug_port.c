#include "debug_port.h"

#ifndef NO_DEBUG_OUT
static int debug_is_present = 0;
#endif

/*
 * Writing to port 0xE9 might (and probably will)
 * cause a freeze or a crash on real hardware.
 * By default this file is built with -DNO_DEBUG_OUT
 * To enable this, compile using "make DEBUG=1"
 */
void debug_write(uint8_t byte) {
	#ifndef NO_DEBUG_OUT
	if (debug_is_present > 0)
		outb(DEBUG_PORT, byte);
	else if (!debug_is_present)
		debug_is_present = inb(DEBUG_PORT) == 0xE9 ? 1 : -1;
	#else
	(void)byte;
	#endif
}
