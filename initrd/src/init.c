/*
 * quack init server
 * */

#include <stdint.h>
#include <stddef.h>

void _start(void) {
	// halt, nothing to do yet

	int i = 0;

	while(1) {
		asm volatile ("int $0x30" : : "a"(i));
		i++;
	}
}
