bits 64

section .data
task_cr3:
	dq 0

section .text
ctx_switch:
	; save cr3
	mov [task_cr3], rsi

	; load ds
	mov rax, [rdi] ; ds
	mov es, ax
	mov ds, ax
	mov fs, ax ; TODO
	mov gs, ax ; TODO

	; load gprs apart from rdi
	mov rax, [rdi + 128]
	mov rbx, [rdi + 120]
	mov rcx, [rdi + 112]
	mov rdx, [rdi + 104]
	mov rbp, [rdi + 96]
	mov rsi, [rdi + 88]
	mov r8, [rdi + 72]
	mov r9, [rdi + 64]
	mov r10, [rdi + 56]
	mov r11, [rdi + 48]
	mov r12, [rdi + 40]
	mov r13, [rdi + 32]
	mov r14, [rdi + 24]
	mov r15, [rdi + 16]

	; set up iret frame
	;push qword [rdi]	; ss
	;push qword [rdi + 144]	; rsp
	push qword [rdi + 152]	; rflags
	push qword [rdi + 8]	; cs
	push qword [rdi + 136]	; rip

	push qword [rdi + 80]	; rdi
	mov rdi, [task_cr3]
	mov cr3, rdi		; load task cr3
	pop rdi			; load dest rdi

	iret
