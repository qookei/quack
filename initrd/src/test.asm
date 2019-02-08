org 0x1000

bits 32

_entry:
	mov dx, 0x3F8

	mov al, 't'
	out dx, al

	mov al, 'e'
	out dx, al

	mov al, 's'
	out dx, al

	mov al, 't'
	out dx, al

	mov al, ':'
	out dx, al

	mov al, ' '
	out dx, al

	mov al, 'h'
	out dx, al

	mov al, 'i'
	out dx, al

	mov al, '!'
	out dx, al

	mov al, 0xA
	out dx, al

	mov al, 0xD
	out dx, al

	xor eax, eax
	xor ebx, ebx
	int 0x30		; sys_exit(0)
