#include "elf.h"
#include <mesg.h>

int elf_check_header(elf_hdr* hdr) {

	if(hdr->mag_num[0] != 0x7f || hdr->mag_num[1] != 'E' || hdr->mag_num[2] != 'L' || hdr->mag_num[3] != 'F') {
		return 0;
	}
	if(hdr->arch != ELF_ARCH_32BIT) {
		return 0;
	}
	if(hdr->byte_order != ELF_BYTEORDER_LENDIAN) {
		return 0;
	}
	if(hdr->elf_ver != 1){
		return 0;
	}
	if(hdr->file_type != ELF_REL && hdr->file_type != ELF_EXEC) {
		return 0;
	}
	if(hdr->machine != ELF_386_MACHINE) {
		return 0;
	}
	return 1;
}

elf_section_header *elf_get_section_header(void *elf_file, int num) {
	elf_hdr *hdr = (elf_hdr *)elf_file;
	return (elf_section_header *)((uint8_t *)(elf_file) + hdr->shoff + hdr->sh_ent_size * num);
}

elf_program_header *elf_get_program_header(void *elf_file, int num) {
	elf_hdr *hdr = (elf_hdr *)elf_file;
	return (elf_program_header *)((uint8_t *)(elf_file)+hdr->phoff+hdr->ph_ent_size*num);
}

const char *elf_get_section_name(void *elf_file, int num) {
	elf_hdr *hdr = (elf_hdr *)elf_file;
	return (hdr->sh_name_index == SH_UNDEF) ? "no section" : (const char*)elf_file + elf_get_section_header(elf_file, hdr->sh_name_index)->offset + elf_get_section_header(elf_file, num)->name;
}


elf_loaded prepare_elf_for_exec(void *elf_file) {
	elf_loaded result;
	result.success_ld = 0;
	result.page_direc = 0;
	result.entry_addr = 0;

	if(!elf_check_header((elf_hdr *)elf_file)){
		early_mesg(LEVEL_INFO, "elf", "failed to verify header");
		return result;
	}

	elf_hdr *hdr = (elf_hdr *)elf_file;

	uint32_t pagedir = create_page_directory();

	for(int i=0; i < hdr->ph_ent_cnt; i++) {
		elf_program_header *phdr = elf_get_program_header(elf_file, i);

		if(phdr->type != SEGTYPE_LOAD) {
			continue;
		}

		uint32_t sz = (phdr->size_in_mem + 0xFFF) / 0x1000;

		if (!alloc_mem_at(pagedir, phdr->load_to & 0xFFFFF000, sz, 0x7)) {
			return result;
		}

		crosspd_memcpy(pagedir, (void *)phdr->load_to, def_cr3(),
						(void *)((uint32_t)elf_file + (phdr->data_offset)),
						phdr->size_in_mem);
	}

	result.page_direc = pagedir;
	result.entry_addr = hdr->entry;
	result.success_ld = 1;

	return result;
}
