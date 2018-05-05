#include "syscall.h"
#include <fs/vfs.h>
#include <tasking/tasking.h>
#include <multiboot.h>

/*

	process control:
	0 - exit()
	1 - fork()										->		pid eax
	2 - execve(path ebx, argv ecx, envp edx)		->		may not return, error eax
	3 - waitpid(pid ebx)							->		return status eax
	14- getpid()									->		pid eax

	vfs:
	4 - open(path ebx, flags ecx)					->		file handle eax
	5 - read(handle ebx, buffer ecx, count edx)		->		size eax
	6 - write(handle ebx, buffer ecx, count edx)	->		size eax
	7 - close(handle ebx)							->		status eax
	10- stat(path ebx, dst ecx)						->		status eax
	11- chdir(path ebx)								->		status eax
	12- getwd(buf ebx)								->		status eax
	13- getcwd(buf ebx, len ecx)					->		status eax
	15- get_ents(path ebx, result ecx, nents edx)	->		status eax

	resources:
	8 - request resource(resource_id ebx)			->		resource address eax or 0xFFFFFFFF
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

extern void mem_dump(void*, size_t, size_t);

extern multiboot_info_t *mbootinfo;

bool verify_addr(uint32_t pd, uint32_t addr, uint32_t len, uint32_t flags) {
	uint32_t caddr = addr;
	bool failed = false;
	
	while (caddr < addr + len) {
		uint32_t fl = get_flag(pd, (void *)caddr);
		if (fl & flags != flags) {
			failed = true;
			break;
		}
		caddr += 0x1000;
	}
	
	return !failed;
}

/*
 * Write src(in kernel) to dst(in user) with size len
 * This function verifies that the destination is valid(user has permissions to
 * access that memory and it's actually mapped)
 * */
bool copy_to_user(void *dst, void *src, size_t len) {
	// verify if dst is valid for the whole len
	if (!dst || !src || !len)
		return false;
	
	if (!verify_addr((uint32_t)current_task->cr3, (uint32_t)dst, len, 0x7)) {
		return false;
	}
	
	crosspd_memcpy(current_task->cr3, dst, def_cr3(), src, len);
	return true;

}

/*
 * Write src(in user) to dst(in kernel) with size len
 * This function verifies that the source is valid(user has permissions to
 * access that memory and it's actually mapped)
 * */
bool copy_from_user(void *dst, void *src, size_t len) {
	// verify if dst is valid for the whole len

	if (!dst || !src || !len)
		return false;
	
	if (!verify_addr((uint32_t)current_task->cr3, (uint32_t)src, len, 0x5)) {
		printf("failed addr %08x", (uint32_t) src);
		return false;
	}
	
	crosspd_memcpy(def_cr3(), dst, current_task->cr3, src, len);
	return true;

}

char buf[1024];

extern char *ps2_low_def;

bool do_syscall(interrupt_cpu_state *state) {

	switch(state->eax) {
		case 0: {
			uint32_t pid = current_task->pid;
			tasking_schedule_next();
			kill_task(pid);
			tasking_schedule_after_kill();
			break;
		}

		case 1: {

			uint32_t fork_stat = tasking_fork(state);
			state->eax = fork_stat;
			break;
		}

		case 2: {
			memset(buf, 0, 1024);
			
			if (!copy_from_user(buf, (void *)state->ebx, 1024)) {
				state->eax = EFAULT;
				break;
			}

			leave_kernel_directory();
			memcpy(buf, (void *)state->ebx, 1024);
			enter_kernel_directory();
			
			int returnval = tasking_execve(buf, NULL, NULL);
			
			if (returnval == -1) {
			
				state->eax = -1;	
				break;
			} else {
			
				tasking_schedule_next();
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

			void *d = kmalloc(1024);

			memset(d, 0, 1024);
			if (!copy_from_user(d, (void *)state->ebx, 1024)) {
			}
			//leave_kernel_directory();
			//memcpy(buf, (void *)state->ebx, strlen((const char *)state->ebx));			
			//enter_kernel_directory();

			state->eax = open((const char *)d, state->ecx);

			break;
		}

		case 5: {

			// kprintf("read\n");

			size_t count = state->edx;

			void *d = kmalloc(count);
			memset(d, 0, count);
			int ret = read(state->ebx, (char *)d, count);
			bool c = copy_to_user((void *)state->ecx, d, count);
			if (!c)
				ret = EFAULT;

			state->eax = ret;
			kfree(d);


			//em_dump(ps2_low_def, 58, 16);

			break;

		}

		case 6: {

			size_t count = state->edx;

			void *d = kmalloc(count);
			memset(d, 0, count);
			bool c = copy_from_user(d, (void *)state->ecx, count);
			if (c) {
				state->eax = write(state->ebx, (char *)d, count);
			} else {
				state->eax = EFAULT;
			}

			kfree(d);
					
			break;
		}

		case 7: {

			// kprintf("close\n");

			state->eax = close(state->ebx);
			break;
		}

		case 8: {
			if (frame_buffer_owner_pid != 0) {
				state->eax = 0xFFFFFFFF;
				break;
			}

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

		case 9: {
			if (frame_buffer_owner_pid == 0) {
				break;
			}

			switch (state->ebx) {
				case 0: {
					uint32_t dst = 0xD0000000;
					uint32_t fbuf_size = mbootinfo->framebuffer_pitch * mbootinfo->framebuffer_height;

					uint32_t pages_to_map = fbuf_size / 4096 + 1;

					leave_kernel_directory();

					for (uint32_t i = 0; i < pages_to_map; i++) {
						unmap_page((void*)(dst));

						dst += 0x1000;

					}

					enter_kernel_directory();



					state->eax = 0;
					frame_buffer_owner_pid = 0;
					break;
				}
			}
			break;

		}

		case 10: {
			// stat
			break;
		}

		case 11: {
			if (!verify_addr(isr_old_cr3, state->ebx, 1024, 0x5)) {
				state->eax = EFAULT;
				break;
			}

			leave_kernel_directory();
			memcpy(buf, (void *)state->ebx, 1024);
			enter_kernel_directory();			

			state->eax = chdir(buf);
			
			break;
		}

		case 12: {
			if (!verify_addr(isr_old_cr3, state->ebx, 1024, 0x7)) {
				state->eax = EFAULT;
				break;
			}

			memset(buf, 0, 1024);

			state->eax = getwd(buf);

			leave_kernel_directory();
			memcpy((void *)state->ebx, buf, strlen(buf) + 1);			
			enter_kernel_directory();
			break;
		}

		case 13: {
			if (!verify_addr(isr_old_cr3, state->ebx, 1024, 0x7)) {
				state->eax = EFAULT;
				break;
			}

			memset(buf, 0, 1024);

			state->eax = getcwd(buf, state->ecx);

			leave_kernel_directory();
			memcpy((void *)state->ebx, buf, strlen(buf) + 1);			
			enter_kernel_directory();
			break;
		}

		case 14: {
			state->eax = current_task->pid;
			break;
		}
		
		case 15: {
			dirent_t *res = NULL;
			if (state->ecx != 0) {
				res = (dirent_t *)kmalloc(state->edx * sizeof(dirent_t));
			}

			copy_from_user(buf, (void *)state->ebx, 1024);

			int ret = get_ents(buf, res);

			if (res != NULL) {
				copy_to_user((void *)state->ecx, res, state->edx * sizeof(dirent_t));
			}

			kfree(res);

			state->eax = ret;

			break;
		}

	}

	
	return true;

}
