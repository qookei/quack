%macro EXC_NO_ERR_CODE_ISR 1
global isr%1
isr%1:
	push 0
	push %1
	jmp service_interrupt
%endmacro

%macro EXC_ERR_CODE_ISR 1
global isr%1
isr%1:
	push %1
	jmp service_interrupt
%endmacro

%macro IRQ_ISR 1
%assign i %1
global isr%1
isr%1:
	push 0
	push %1
	jmp service_interrupt
%endmacro

%macro OTHER_ISR 1
global isr%1
isr%1:
	push 0
	push %1
	jmp service_interrupt
%endmacro

EXC_NO_ERR_CODE_ISR 0
EXC_NO_ERR_CODE_ISR 1
EXC_NO_ERR_CODE_ISR 2
EXC_NO_ERR_CODE_ISR 3
EXC_NO_ERR_CODE_ISR 4
EXC_NO_ERR_CODE_ISR 5
EXC_NO_ERR_CODE_ISR 6
EXC_NO_ERR_CODE_ISR 7
EXC_ERR_CODE_ISR 8
EXC_NO_ERR_CODE_ISR 9
EXC_ERR_CODE_ISR 10
EXC_ERR_CODE_ISR 11
EXC_ERR_CODE_ISR 12
EXC_ERR_CODE_ISR 13
EXC_ERR_CODE_ISR 14
EXC_NO_ERR_CODE_ISR 15
EXC_NO_ERR_CODE_ISR 16
EXC_ERR_CODE_ISR 17
EXC_NO_ERR_CODE_ISR 18
EXC_NO_ERR_CODE_ISR 19
EXC_NO_ERR_CODE_ISR 20
EXC_NO_ERR_CODE_ISR 21
EXC_NO_ERR_CODE_ISR 22
EXC_NO_ERR_CODE_ISR 23
EXC_NO_ERR_CODE_ISR 24
EXC_NO_ERR_CODE_ISR 25
EXC_NO_ERR_CODE_ISR 26
EXC_NO_ERR_CODE_ISR 27
EXC_NO_ERR_CODE_ISR 28
EXC_NO_ERR_CODE_ISR 29
EXC_ERR_CODE_ISR 30
EXC_NO_ERR_CODE_ISR 31

IRQ_ISR 32
IRQ_ISR 33
IRQ_ISR 34
IRQ_ISR 35
IRQ_ISR 36
IRQ_ISR 37
IRQ_ISR 38
IRQ_ISR 39
IRQ_ISR 40
IRQ_ISR 41
IRQ_ISR 42
IRQ_ISR 43
IRQ_ISR 44
IRQ_ISR 45
IRQ_ISR 46
IRQ_ISR 47

OTHER_ISR 48		; syscall

EXTERN dispatch_interrupt

service_interrupt:
	push eax
	push ebx
	push ecx
	push edx
	push ebp
	push esi
	push edi

	xor eax, eax
	mov ax, ds
	push eax

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call dispatch_interrupt

	pop eax

	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	pop edi
	pop esi
	pop ebp
	pop edx
	pop ecx
	pop ebx
	pop eax

	add esp, 8
	iret
