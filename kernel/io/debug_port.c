#include "debug_port.h"

/*
 * Writing to port 0xE9 might (and probably will)
 * cause a freeze or a crash on real hardware.
 * By default this file is built with -DNO_DEBUG_OUT
 * To enable this, compile using "make DEBUG=1"
 */
void debug_write(uint8_t byte) {
	#ifndef NO_DEBUG_OUT
	outb(DEBUG_PORT, byte);
	#else
	(void)byte;
	#endif
}
