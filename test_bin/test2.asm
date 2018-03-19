bits 32

org 0x1000

; _start:
	; mov esi, 'A'
; _loop:
	; call _putchar
	
	; add esi, 1
	; cmp esi, '~'
	; jge _crash
	; jmp _loop
	
_crash:
	mov eax, [0xC0000000]
	xor eax, 0x10101010
	mov [0xC0000000], eax
	jmp _crash


_putchar:
	mov eax, 2
	int 0x30
	ret
