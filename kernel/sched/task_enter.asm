[bits 32]
global task_enter
global task_idle

task_cr3:
	dd 0

; eax - cr3
; ebx - cpu_state_t

task_enter:
    mov dword [task_cr3], eax

    mov eax, dword [ebx]
    mov es, ax
    mov ds, ax
    mov fs, ax
    mov gs, ax

    mov eax, dword [ebx+4]
    mov ecx, dword [ebx+12]
    mov edx, dword [ebx+16]
    mov esi, dword [ebx+20]
    mov edi, dword [ebx+24]
    mov ebp, dword [ebx+28]

    push dword [ebx+48]
    push dword [ebx+44]
    push dword [ebx+40]
    push dword [ebx+36]
    push dword [ebx+32]

    push dword [ebx+8]
    mov ebx, dword [task_cr3]
    mov cr3, ebx
    pop ebx

    iret

extern isr_stack

task_idle:
	mov esp, isr_stack
	
	push 0x202
	push 0x08
	push idle_loop
	iret

idle_loop:
	hlt
	jmp idle_loop
