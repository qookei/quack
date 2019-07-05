section .trampoline
bits 16

global smp_entry

smp_entry:
	cli
	xor ax,ax
	mov ds,ax

	mov dx, 0x3F8

	mov al, '1'
	out dx, al

	jmp 0x0000:.reset_cs
.reset_cs:
	mov al, '2'
	out dx, al
	hlt
