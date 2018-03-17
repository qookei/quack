bits 32

org 0x1000

_start:
	mov esi, 'A'
_loop:
	push esi
	push 10
	push _str1
	call _putstr
	pop esi

	call _putchar
	push esi
	mov esi, 0xA
	call _putchar
	pop esi
	
	add esi, 1
	cmp esi, '~'
	jge _crash
	jmp _loop
	
_crash:
	mov eax, [0xC0000000]
	xor eax, 0x10101010
	mov [0xC0000000], eax
	jmp _crash


_putchar:
	mov eax, 2
	int 0x30
	ret

_putstr:
	pop eax
	pop esi
	pop ecx
	push eax 	; save return address
	mov eax, 0
	int 0x30
	ret

_str1:
	db "Prg 2 - ", 0x0
