bits 64

extern vmm_drop_context

global internal_task_idle
internal_task_idle:
	mov rsp, rdi

	push 0x0
	push rdi
	push 0x202
	push 0x08
	push .idle_loop
	iretq

.idle_loop:
	hlt
	jmp .idle_loop
