#include "elf.h"

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
