#include "elf64.h"
#include <string.h>

static bool elf64_check(elf64_ehdr *ehdr) {
	if(strncmp((const char *)ehdr->e_ident, ELF_MAGIC, 4)) {
		return false;
	}

	if(ehdr->e_ident[ELF_IDENT_CLASS] != ELF_CLASS_64) {
		return false;
	}

	if(ehdr->e_ident[ELF_IDENT_DATA] != ELF_DATA_LSB) {
		return false;
	}

	if(ehdr->e_version != ELF_VER_CURRENT){
		return false;
	}

	if(ehdr->e_type != ELF_TYPE_EXEC) {
		return false;
	}

	return true;
}

bool elf64_load(void *file, thread &t) {
	elf64_ehdr *ehdr = (elf64_ehdr *)file;

	if (!elf64_check(ehdr))
		return false;


	elf64_phdr *phdrs = (elf64_phdr *)((uintptr_t)file + ehdr->e_phoff);

	for(int i = 0; i < ehdr->e_phnum; i++) {
		if(phdrs[i].p_type != ELF_PHDR_TYPE_LOAD) {
			continue;
		}

		int flags = vm_perm::user;
		if (phdrs[i].p_flags & ELF_PHDR_FLAG_X)
			flags |= vm_perm::execute;
		if (phdrs[i].p_flags & ELF_PHDR_FLAG_R)
			flags |= vm_perm::read;
		if (phdrs[i].p_flags & ELF_PHDR_FLAG_W)
			flags |= vm_perm::write;

		size_t n_pages = (phdrs[i].p_memsz + vm_page_size - 1) / vm_page_size;

		t._addr_space->allocate_exact_lazy(phdrs[i].p_vaddr, n_pages, flags);
		t._addr_space->load(phdrs[i].p_vaddr,
			(void *)((uintptr_t)file + phdrs[i].p_offset),
			phdrs[i].p_filesz);
	}

	t._task->ip() = ehdr->e_entry;

	return true;
}
