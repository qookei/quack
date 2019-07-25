bits 64

section .data
task_cr3:
	dq 0

section .text
global ctx_switch
ctx_switch:
	; save cr3
	mov [task_cr3], rsi

	; mov fs, ax ; TODO
	; mov gs, ax ; TODO

	xor rax, rax
	mov es, ax
	mov ds, ax

	; load gprs apart from rdi
	mov r15, [rdi]
	mov r14, [rdi + 8]
	mov r13, [rdi + 16]
	mov r12, [rdi + 24]
	mov r11, [rdi + 32]
	mov r10, [rdi + 40]
	mov r9,  [rdi + 48]
	mov r8,  [rdi + 56]
	; rdi is at [rdi + 64]
	mov rsi, [rdi + 72]
	mov rbp, [rdi + 80]
	mov rdx, [rdi + 88]
	mov rcx, [rdi + 96]
	mov rbx, [rdi + 104]
	mov rax, [rdi + 112]

	; set up iret frame
	push qword [rdi + 120]	; ss
	push qword [rdi + 128]	; rsp
	push qword [rdi + 136]	; rflags
	push qword [rdi + 144]	; cs
	push qword [rdi + 152]	; rip

	push qword [rdi + 64]	; rdi
	mov rdi, [task_cr3]
	mov cr3, rdi		; load task cr3
	pop rdi			; load dest rdi

	iretq
