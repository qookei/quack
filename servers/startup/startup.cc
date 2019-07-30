#include <stdint.h>

void sys_debug_log(char val) {
	asm volatile("int $0x30" : : "a"(1), "b"(val));
}

void str_put(char *s) {
	while(*s) {
		sys_debug_log(*s);
		s++;
	}
}

extern "C" {
	void _start(void) {
		str_put("hello, userspace world!\n");
		while(1);
	}
}
