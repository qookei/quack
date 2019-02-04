global _loader
extern kernel_main

MODULEALIGN equ  1<<0
MEMINFO		equ  1<<1
VIDEO		equ  1<<2
FLAGS		equ  MODULEALIGN | MEMINFO | VIDEO
MAGIC		equ    0x1BADB002
CHECKSUM	equ -(MAGIC + FLAGS)

section .multiboot
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

	; keep it all 0 since this is not A.OUT

	dd 0
	dd 0
	dd 0
	dd 0
	dd 0

	; video mode

	dd 0
	dd 0
	dd 0
	dd 0

KERNEL_VIRTUAL_BASE equ 0xC0000000
KERNEL_PAGE_NUMBER equ (KERNEL_VIRTUAL_BASE >> 22)

%macro page_table_creat 1
	%assign i %1
	%rep 1024
		dd (i | 3)
		%assign i i+0x1000
	%endrep
%endmacro

section .data

align 0x1000
page_tab1:
	page_table_creat 0x00000000

align 0x1000
page_tab2:
	page_table_creat 0x00400000

align 0x1000
page_tab3:
	page_table_creat 0x00800000

global page_tab1
global page_tab2
global page_tab3

align 0x1000
page_dir:
	dd (page_tab1 - KERNEL_VIRTUAL_BASE)
	dd (page_tab2 - KERNEL_VIRTUAL_BASE)
	dd (page_tab3 - KERNEL_VIRTUAL_BASE)
	times (KERNEL_PAGE_NUMBER - 3) dd 0
	dd (page_tab1 - KERNEL_VIRTUAL_BASE)
	dd (page_tab2 - KERNEL_VIRTUAL_BASE)
	dd (page_tab3 - KERNEL_VIRTUAL_BASE)
	times (1024 - KERNEL_PAGE_NUMBER - 3) dd 0

section .text
align 4

STACKSIZE equ 0x4000

global loader
loader equ (_loader - 0xC0000000)

_loader:
	; load page directory
	mov ecx, (page_dir - KERNEL_VIRTUAL_BASE)
	mov cr3, ecx

	; set permission bits for first 3 kernel entries
	mov eax, [ecx]
	or eax, 3
	mov [ecx], eax
	add ecx, 4

	mov eax, [ecx]
	or eax, 3
	mov [ecx], eax
	add ecx, 4

	mov eax, [ecx]
	or eax, 3
	mov [ecx], eax

	; set permission bits for later 3 kernel entries
	mov ecx, (page_dir - KERNEL_VIRTUAL_BASE)
	add ecx, (KERNEL_PAGE_NUMBER * 4)

	mov eax, [ecx]
	or eax, 3
	mov [ecx], eax
	add ecx, 4

	mov eax, [ecx]
	or eax, 3
	mov [ecx], eax
	add ecx, 4

	mov eax, [ecx]
	or eax, 3
	mov [ecx], eax

	; enable paging
	mov ecx, cr0
	or ecx, 0x80000000
	mov cr0, ecx

	lea ecx, [higher_half]
	jmp ecx

higher_half:
	mov dword [page_dir], 0
	mov dword [page_dir + 4], 0
	mov dword [page_dir + 8], 0

	mov ecx, (page_dir - KERNEL_VIRTUAL_BASE)
	mov cr3, ecx

	mov esp, stack+STACKSIZE

	add ebx, 0xC0000000
	push ebx

	call  kernel_main
l:
	hlt
	jmp l

section .bss
align 32
stack:
	resb STACKSIZE		; reserve 16k stack on a uint64_t boundary
