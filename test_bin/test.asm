bits 32

org 0x1000

_start:
	mov eax, 0
	mov esi, _str1
	mov ecx, 18
	int 0x30
	mov esi, _str3
	mov ecx, 21
	int 0x30
	
_loop:
	call getch

	mov eax, 2
	mov esi, edx
	int 0x30
	jmp _loop

readch:
	mov eax, 1
	int 0x30
	ret

getch:
	call readch
	cmp edx, 0
	je getch
	ret

_str1:
	db "Usermode Notepad", 0xA, 0x0

_str3:
	db "You can type here: ", 0xA, 0x0