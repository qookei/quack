#include "elf64.h"
#include <kmesg.h>
#include <string.h>
#include <arch/task.h>
#include <arch/mm.h>

int elf64_check(void *file) {
	elf64_ehdr *ehdr = file;

	if(strncmp((const char *)ehdr->e_ident, ELF_MAGIC, 4)) {
		return 1;
	}
	if(ehdr->e_ident[ELF_IDENT_CLASS] != ELF_CLASS_64) {
		return 2;
	}
	if(ehdr->e_ident[ELF_IDENT_DATA] != ELF_DATA_LSB) {
		return 3;
	}
	if(ehdr->e_version != ELF_VER_CURRENT){
		return 4;
	}
	if(ehdr->e_type != ELF_TYPE_EXEC) {
		return 5;
	}
	return 0;
}

arch_task_t *elf64_create_arch_task(void *file) {
	elf64_ehdr *ehdr = file;

	arch_task_t *task = arch_task_create_new(NULL);

	elf64_phdr *phdrs = (elf64_phdr *)((uintptr_t)file + ehdr->e_phoff);

	for(int i = 0; i < ehdr->e_phnum; i++) {
		if(phdrs[i].p_type != ELF_PHDR_TYPE_LOAD) {
			continue;
		}

		struct mem_region region = {
			.dest = 0,
			.start = phdrs[i].p_vaddr,
			.end = phdrs[i].p_vaddr + phdrs[i].p_memsz,
		};

		int flags = ARCH_MM_FLAG_U;
		if (phdrs[i].p_flags & ELF_PHDR_FLAG_X)
			flags |= ARCH_MM_FLAG_E;
		if (phdrs[i].p_flags & ELF_PHDR_FLAG_R)
			flags |= ARCH_MM_FLAG_R;
		if (phdrs[i].p_flags & ELF_PHDR_FLAG_W)
			flags |= ARCH_MM_FLAG_W;

		arch_task_alloc_mem_region(task, &region, flags);
		arch_task_copy_to_mem_region(task, &region, 
			(void *)((uintptr_t)file + phdrs[i].p_offset),
			phdrs[i].p_filesz);
	}

	arch_task_load_entry_point(task, ehdr->e_entry);

	return task;
}
