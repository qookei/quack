[bits 32]
[org 0x1000]

start:
	mov eax, 6
	mov ebx, 1
	mov ecx, _str1
	mov edx, 56
	int 0x30

_loop:
	mov eax, 5
	mov ebx, 0
	mov ecx, _buf
	mov edx, 16
	int 0x30
	mov esi, eax
	cmp eax, 0
	je _loop
	mov eax, 6
	mov ebx, 1
	mov ecx, _buf
	mov edx, esi
	int 0x30
	jmp _loop

_str1:
	db "Usermode notepad v2.0", 0xA, "Using filesystem read/write calls", 0xA

_buf:
	times 16 db 0