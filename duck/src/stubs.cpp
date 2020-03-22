#include <duck/calls.h>
#include <duck/syscall.h>

duck_error_t duck_set_identity(const char *ident, size_t length) {
	return duck_syscall_2(duck_call_set_identity, (uintptr_t)ident, length);
}

duck_error_t duck_debug_log(const char *message, size_t length) {
	return duck_syscall_2(duck_call_debug_log, (uintptr_t)message, length);
}

// ---------------------------------------------------------------------

duck_error_t duck_syscall_2(int syscall_no, uintptr_t a0, uintptr_t a1) {
	int tmp = syscall_no;
	asm volatile ("int $0x30" : "+a"(tmp) : "b"(a0), "c"(a1));
	return tmp;
}
