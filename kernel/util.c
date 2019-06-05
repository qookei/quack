#include "util.h"

#include <kmesg.h>

void __assert_failure(const char *expr, const char *file, int line, const char *function) {
	kmesg("kernel", "%s:%d: %s: assertion '%s' failed", file, line, function, expr);
	__builtin_trap();
}
