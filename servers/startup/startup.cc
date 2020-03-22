#include <stdint.h>

#include <duck/error.h>
#include <duck/calls.h>

extern "C" {
	void _start(void) {
		duck_set_identity("startup", 7);
		duck_debug_log("hello, userspace world!", 23);
		DUCK_CHECK(duck_debug_log(nullptr, 1024));
		while(1);
	}
}
