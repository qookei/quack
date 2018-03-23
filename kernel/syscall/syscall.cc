#include "syscall.h"
#include "../fs/vfs.h"

/*

	system calls as of now:
	0 - tty_writestr((char*)esi, ecx)
	1 - kb_rd() -> edx
	2 - tty_putchar(esi)
	3 - reset() dont use, for testing

	vfs
	4 - open(path ebx, flags ecx)					->		file handle eax
	5 - read(handle ebx, buffer ecx, count edx)		->		size eax
	6 - write(handle ebx, buffer ecx, count edx)	->		size eax
	7 - close(handle ebx)							->		status eax

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
extern int kprintf(const char*, ...);

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
		case 2: {
			char c = (char)state->esi;

			tty_putchar(c);
			break;
		}
		case 3:
			outb(0x64, 0xFE);
			break;


		case 4: {
			// open
			leave_kernel_directory();
			void* phys = get_phys((void *)state->ebx);
			enter_kernel_directory();
			kprintf("%p\n", phys);

			map_page((void*)((uint32_t)phys & 0xFFFFF000), (void *)0xEFFFF000, 0x3);
			state->eax = open((char *)(0xEFFFF000 + (((uint32_t)phys) & 0xFFF)), state->ecx);
			unmap_page((void *)0xEFFFF000);
			kprintf("%i\n", state->eax);
			break;
		}

		case 5: {
			// read
			kprintf("usr\n");
			leave_kernel_directory();
			void* phys = get_phys((void *)state->ecx);
			uint32_t off = ((uint32_t)phys) & 0xFFF;
			phys = (void*)(((uint32_t)phys) & 0xFFFFF000);
			enter_kernel_directory();

			size_t count = state->edx;
			uint32_t pages = count / 0x1000;
			if ((count % 0x1000) != 0) pages++;

			for (uint32_t i = 0; i < count; i++) {
				map_page((void*)((uint32_t)phys + (i * 0x1000)), ((void *)(0xE0000000 + i * 0x1000)), 0x3);
			}

			state->eax = read(state->ebx, (char *)(0xE0000000 + off), count);

			for (uint32_t i = 0; i < count; i++) {
				unmap_page((void *)(0xE0000000 + i * 0x1000));
			}
			break;

		}

		case 6: {
			// write
			
			kprintf("usr write\n");
			leave_kernel_directory();
			void* phys = get_phys((void *)state->ecx);
			uint32_t off = ((uint32_t)phys) & 0xFFF;
			phys = (void*)(((uint32_t)phys) & 0xFFFFF000);
			enter_kernel_directory();

			size_t count = state->edx;
			uint32_t pages = count / 0x1000;
			if ((count % 0x1000) != 0) pages++;

			for (uint32_t i = 0; i < count; i++) {
				map_page((void*)((uint32_t)phys + (i * 0x1000)), ((void *)(0xE0000000 + i * 0x1000)), 0x3);
			}

			state->eax = write(state->ebx, (char *)(0xE0000000 + off), count);

			for (uint32_t i = 0; i < count; i++) {
				unmap_page((void *)(0xE0000000 + i * 0x1000));
			}
			break;
		}

		case 7: {
			// close
			kprintf("usr close\n");
			// leave_kernel_directory();
			state->eax = close(state->ebx);
			break;
		}
	}

	
	return true;

}