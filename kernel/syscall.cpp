#include "syscall.h"

#include <arch/io.h>

#include <duck/calls.h>
#include <duck/types.h>
#include <duck/error.h>

#include <kmesg.h>

#include <proc/scheduler.h>

uintptr_t syscall_invoke(uintptr_t *arg0, uintptr_t *arg1,
		uintptr_t *arg2, uintptr_t *arg3,
		uintptr_t *arg4, uintptr_t *arg5,
		void *irq_state) {

	(void)arg3;
	(void)arg4;
	(void)arg5;
	(void)irq_state;

	switch(*arg0) {
		case duck_call_set_identity:
			return duck_set_identity((const char *)(*arg1), (size_t)(*arg2));
		case duck_call_debug_log:
			return duck_debug_log((const char *)(*arg1), (size_t)(*arg2));
		default:
			return duck_error_invalid_call;
	}
}

duck_error_t duck_set_identity(const char *ident, size_t length) {
	kmesg("syscall", "in duck_set_identity");

	auto thread = sched_this();

	if (length > 64)
		return duck_error_out_of_range;

	thread->_identity.resize(length);
	thread->_addr_space->store((uintptr_t)ident, thread->_identity.data(), length);

	return duck_error_none;
}

duck_error_t duck_debug_log(const char *message, size_t length) {
	kmesg("syscall", "in duck_debug_log");

	auto thread = sched_this();

	if (length > 512)
		return duck_error_out_of_range;

	char buf[length + 1];
	thread->_addr_space->store((uintptr_t)message, buf, length);
	buf[length] = 0;

	kmesg(thread->_identity.data(), "%s", buf);

	return duck_error_none;
}

