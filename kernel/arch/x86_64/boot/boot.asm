global _loader
extern arch_entry

MODULEALIGN equ  1<<0
MEMINFO		equ  1<<1
VIDEO		equ  1<<2
FLAGS		equ  MODULEALIGN | MEMINFO | VIDEO
MAGIC		equ    0x1BADB002
CHECKSUM	equ -(MAGIC + FLAGS)

bits 32

section .multiboot
align 64
mboot:
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
	dd 32

KERNEL_VIRTUAL_BASE equ 0xFFFFFFFF80000000

section .data
align 0x1000
init_pml4:
	dq init_pdp1 - KERNEL_VIRTUAL_BASE + 3
	times 255 dq 0
	dq phys_pdp - KERNEL_VIRTUAL_BASE + 3
	times 254 dq 0
	dq init_pdp2 - KERNEL_VIRTUAL_BASE + 3

%macro gen_pd_2mb 3
	%assign i %1
	%rep %2
		dq (i | 0x83)
		%assign i i+0x200000
	%endrep
	%rep %3
		dq 0
	%endrep
%endmacro

align 0x1000
init_pdp1:
	dq init_pd - KERNEL_VIRTUAL_BASE + 3
	times 511 dq 0

align 0x1000
init_pdp2:
	times 510 dq 0
	dq init_pd - KERNEL_VIRTUAL_BASE + 3
	dq 0

align 0x1000
phys_pdp:
	dq phys_pd - KERNEL_VIRTUAL_BASE + 3
	times 511 dq 0

align 0x1000
init_pd:
	gen_pd_2mb 0,64,448

align 0x1000
phys_pd:
	gen_pd_2mb 0,512,0

align 0x10
init_gdt:
	dq 0
	dq 0x00209A0000000000
init_gdt_end:

init_gdt_ptr:
	dw init_gdt_end - init_gdt - 1
	dq init_gdt - KERNEL_VIRTUAL_BASE

init_gdt_ptr_high:
	dw init_gdt_end - init_gdt - 1
	dq init_gdt

section .text
align 4

global loader
loader equ (_loader - KERNEL_VIRTUAL_BASE)

_loader:
	lgdt [init_gdt_ptr - KERNEL_VIRTUAL_BASE]

	mov esp, stack_end - KERNEL_VIRTUAL_BASE
	
	push 0		; fix stack for later
	push eax
	push 0		; fix stack for later
	push ebx 

	mov eax, cr4
	or eax, 0x000000A0
	mov cr4, eax

	mov eax, init_pml4 - KERNEL_VIRTUAL_BASE
	mov cr3, eax

	mov ecx, 0xC0000080
	rdmsr
	or eax, 0x00000901
	wrmsr

	mov eax, cr0
	or eax, 0x80000001
	mov cr0, eax

	jmp 0x08:higher_half_entry_lower

higher_half_entry_lower equ (higher_half_entry - KERNEL_VIRTUAL_BASE)

bits 64
higher_half_entry:
	mov rax, higher_half
	jmp rax

higher_half:
	lgdt [init_gdt_ptr_high]

	mov ax, 0x0
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov gs, ax
	mov fs, ax

	add rsp, KERNEL_VIRTUAL_BASE
	pop rdi
	pop rsi
	call arch_entry

	hlt

section .bss

STACKSIZE equ 0x10000

global stack

align 0x1000
stack:
	resb STACKSIZE
stack_end:
