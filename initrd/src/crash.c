#include <stdint.h>
#include <stddef.h>

void fork() {
	asm ("int $0x30" : : "a"(1));
}

void _start(void) {
	
	while(1) {
		fork();
	}
	
}
