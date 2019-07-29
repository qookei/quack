bits 64
global arch_mem_fast_memcpy
arch_mem_fast_memcpy:
	push rdi
	push rsi
	push rdx
	mov rcx, rdx
	rep movsb
	pop rdx
	pop rsi
	pop rdi
	ret

global arch_mem_fast_memset
arch_mem_fast_memset:
	push rax
	push rdi
	push rsi
	push rdx
	mov rcx, rdx
	mov rax, rsi
	rep stosb
	pop rdx
	pop rsi
	pop rdi
	pop rax
	ret
