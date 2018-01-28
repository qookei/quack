#include "syscall.h"

/*

	system calls as of now:
	0 - tty_writestr((char*)esi, ecx)
	1 - kb_rd() -> edx
	2 - tty_putchar(esi)

*/

/*

	how to use the kernel directory:
	read parameters from usermode(if it's a structure, string or whatnot)
	switch to kernel dir(if needs screen memory/kernel heap/kernel structures)
	run whatever you need
	switch back(it doesn't waste processing time if not actually switched)
	exit from syscall

*/

bool do_syscall(interrupt_cpu_state*);

void syscall_init() {
	register_interrupt_handler(0x30, do_syscall);
}

extern size_t strlen(const char*);
extern void* memcpy(void*, const void*, size_t);
extern void* memset(void*, int, size_t);

bool do_syscall(interrupt_cpu_state *state) {

	switch(state->eax) {
		case 0: {
			leave_kernel_directory();
			uint32_t ss = state->ecx;
			char buf[ss];
			memcpy(buf,(void*)state->esi, ss);
			enter_kernel_directory();
			tty_putstr(buf);
			break;
		}
		case 1:
			state->edx = readch();
			break;
		case 2:
			char c = (char)state->esi;

			tty_putchar(c);
			break;
	}

	
	return true;

}