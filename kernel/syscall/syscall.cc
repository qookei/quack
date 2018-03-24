#include "syscall.h"
#include "../fs/vfs.h"

/*

	system calls as of now:
	0 - exit()
	1 - unimpl
	2 - unimpl
	3 - unimpl

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

extern file_handle_t *files;
extern task_t *current_task;

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
			uint32_t exit_pid = current_task->pid;
			tasking_schedule_next();
			kill_task(exit_pid);
			break;
		}
		case 1:
		case 2:
		case 3:
			break;


		case 4: {
			// open
			leave_kernel_directory();
			void* phys = get_phys((void *)state->ebx);
			enter_kernel_directory();

			map_page((void*)((uint32_t)phys & 0xFFFFF000), (void *)0xEFFFF000, 0x3);
			state->eax = open((char *)(0xEFFFF000 + (((uint32_t)phys) & 0xFFF)), state->ecx);
			unmap_page((void *)0xEFFFF000);
			break;
		}

		case 5: {
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
			
			int desc = state->ebx;

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

			state->eax = write(desc, (char *)(0xE0000000 + off), count);

			for (uint32_t i = 0; i < count; i++) {
				unmap_page((void *)(0xE0000000 + i * 0x1000));
			}
			
			break;
		}

		case 7: {
			state->eax = close(state->ebx);
			break;
		}
	}

	
	return true;

}