#include "syscall.h"
#include <fs/vfs.h>
#include <tasking/tasking.h>
#include <multiboot.h>

/*

	process control:
	0 - exit()
	1 - fork() 										->	 	pid eax
	2 - execve(path ebx, argv ecx, envp edx)		->		may not return, error eax
	3 - waitpid(pid ebx)							->		blocks until process at pid exits, return status eax

	vfs:
	4 - open(path ebx, flags ecx)					->		file handle eax
	5 - read(handle ebx, buffer ecx, count edx)		->		size eax
	6 - write(handle ebx, buffer ecx, count edx)	->		size eax
	7 - close(handle ebx)							->		status eax
	10- stat(path ebx, dst ecx)						-> 		status eax
	11- chdir(path ebx)								->		status eax
	12- getwd(buf ebx)								->		status eax
	13- getcwd(buf ebx, len ecx)					->		status eax

	resources:
	8 - request resource(resource_id ebx)			->		resource address eax(0xFFFFFFFF on error)
		0 - frame buffer address
	9 - free resource(resource_id)					->		status eax(0 on success, not changed otherwise)
*/

/*

	TODO: Find a better way to copy from and to usermode
	Validate if we're not accessing kernel space or NULL memory

*/

uint32_t frame_buffer_owner_pid = 0;

extern file_handle_t *files;
extern task_t *current_task;

bool do_syscall(interrupt_cpu_state*);

void syscall_init() {
	register_interrupt_handler(0x30, do_syscall);
}

extern int kprintf(const char*, ...);
extern int printf(const char*, ...);

extern uint32_t isr_old_cr3;

extern multiboot_info_t *mbootinfo;

bool do_syscall(interrupt_cpu_state *state) {

	switch(state->eax) {
		case 0: {
			uint32_t pid = current_task->pid;
			printf("Process %u has quit!\n", pid);
			tasking_schedule_next();
			kill_task(pid);
			tasking_schedule_after_kill();
			break;
		}

		case 1: {

			// kprintf("fork\n");

			uint32_t fork_stat = tasking_fork(state);
			state->eax = fork_stat;
			break;
		}

		case 2: {

			// kprintf("exec\n");

			leave_kernel_directory();

			void* phys = get_phys((void *)state->ebx);
			enter_kernel_directory();

			map_page((void*)((uint32_t)phys & 0xFFFFF000), (void *)0xEFFFF000, 0x3);

			int returnval = tasking_execve((const char *)(0xEFFFF000 + ((uint32_t)phys & 0xFFF)), NULL, NULL);
			if (returnval == -1) {
				unmap_page((void *)0xEFFFF000);
				state->eax = -1;
				break;
			} else {
				unmap_page((void *)0xEFFFF000);
				tasking_schedule_next();
				// printf("successfully exec\n");
				tasking_schedule_after_kill();
				break;
			}
			break;
		}



		case 3: {
			tasking_waitpid(state, state->ebx);
			tasking_schedule_next();
			tasking_schedule_after_kill();
			break;
		}


		case 4: {
			// open

			// kprintf("open\n");

			leave_kernel_directory();
			void* phys = get_phys((void *)state->ebx);
			enter_kernel_directory();

			map_page((void*)((uint32_t)phys & 0xFFFFF000), (void *)0xE0000000, 0x3);
			state->eax = open((char *)(0xE0000000 + (((uint32_t)phys) & 0xFFF)), state->ecx);
			unmap_page((void *)0xE0000000);
			break;
		}

		case 5: {

			// kprintf("read\n");

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
			
			// kprintf("write\n");

			int desc = state->ebx;

			leave_kernel_directory();

			void* phys = get_phys((void *)state->ecx);

			uint32_t off = ((uint32_t)phys) & 0xFFF;
			phys = (void*)(((uint32_t)phys) & 0xFFFFF000);

			enter_kernel_directory();
			// kprintf("ctask pid %u\n", current_task->pid);

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

			// kprintf("close\n");

			state->eax = close(state->ebx);
			break;
		}

		case 8: {
			// if (frame_buffer_owner_pid != 0) {
			// 	state->eax = 0xFFFFFFFF;
			// 	break;
			// }

			switch(state->ebx) {
				case 0: {
					// frame buffer address

					uint32_t addr = mbootinfo->framebuffer_addr;
					addr &= 0xFFFFF000;
					
					uint32_t dst = 0xD0000000;
					uint32_t fbuf_size = mbootinfo->framebuffer_pitch * mbootinfo->framebuffer_height;
					
					uint32_t pages_to_map = fbuf_size / 4096 + 1;
					
					leave_kernel_directory();

					for (uint32_t i = 0; i < pages_to_map; i++) {
						map_page((void*)(addr), (void*)(dst), 7);
					
						addr += 0x1000;
						dst += 0x1000;

					}

					enter_kernel_directory();

					uint32_t off = (uint32_t)mbootinfo->framebuffer_addr & 0xFFF;

					state->eax = 0xD0000000 + off;
					break;
				}
				default:
					state->eax = 0xFFFFFFFF;
			}

			break;
		}

		case 10: {
			// stat
			break;
		}

		case 11: {
			leave_kernel_directory();
			void* phys = get_phys((void *)state->ebx);
			enter_kernel_directory();

			map_page((void*)((uint32_t)phys & 0xFFFFF000), (void *)0xE0000000, 0x3);
			state->eax = chdir((const char *)(0xE0000000 + (((uint32_t)phys) & 0xFFF)));
			unmap_page((void *)0xE0000000);
			break;
		}

		case 12: {
			leave_kernel_directory();
			void* phys = get_phys((void *)state->ebx);
			enter_kernel_directory();

			map_page((void*)((uint32_t)phys & 0xFFFFF000), (void *)0xE0000000, 0x3);
			state->eax = getwd((char *)(0xE0000000 + (((uint32_t)phys) & 0xFFF)));
			unmap_page((void *)0xE0000000);
			break;
		}

		case 13: {
			leave_kernel_directory();
			void* phys = get_phys((void *)state->ebx);
			enter_kernel_directory();

			map_page((void*)((uint32_t)phys & 0xFFFFF000), (void *)0xE0000000, 0x3);
			state->eax = getcwd((char *)(0xE0000000 + (((uint32_t)phys) & 0xFFF)), state->ecx);
			unmap_page((void *)0xE0000000);
		}
	}

	
	return true;

}