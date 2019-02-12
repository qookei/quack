#include "debug_port.h"

void debug_write(uint8_t byte) {
	outb(DEBUG_PORT, byte);
}
