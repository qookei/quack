global isr0
isr0:
	push 0
	push 0
	jmp service_interrupt

global isr1
isr1:
	push 0
	push 1
	jmp service_interrupt
	
global isr2
isr2:
	push 0
	push 2
	jmp service_interrupt
	
global isr3
isr3:
	push 0
	push 3
	jmp service_interrupt
	
global isr4
isr4:
	push 0
	push 4
	jmp service_interrupt
	
global isr5
isr5:
	push 0
	push 5
	jmp service_interrupt
	
global isr6
isr6:
	push 0
	push 6
	jmp service_interrupt
	
global isr7
isr7:
	push 0
	push 7
	jmp service_interrupt
	
global isr8
isr8:
	push 8
	jmp service_interrupt
	
global isr9
isr9:
	push 0
	push 9
	jmp service_interrupt
	
global isr10
isr10:
	push 10
	jmp service_interrupt
	
global isr11
isr11:
	push 11
	jmp service_interrupt
	
global isr12
isr12:
	push 12
	jmp service_interrupt
	
global isr13
isr13:
	push 13
	jmp service_interrupt
	
global isr14
isr14:
	push 14
	jmp service_interrupt
	
global isr15
isr15:
	push 0
	push 15
	jmp service_interrupt
	
global isr16
isr16:
	push 0
	push 16
	jmp service_interrupt
	
global isr17
isr17:
	push 0
	push 17
	jmp service_interrupt
	
global isr18
isr18:
	push 0
	push 18
	jmp service_interrupt
	
global isr19
isr19:
	push 0
	push 19
	jmp service_interrupt
	
global isr20
isr20:
	push 0
	push 20
	jmp service_interrupt
	
global isr21
isr21:
	push 0
	push 21
	jmp service_interrupt
	
global isr22
isr22:
	push 0
	push 22
	jmp service_interrupt
		
global isr23
isr23:
	push 0
	push 23
	jmp service_interrupt
		
global isr24
isr24:
	push 0
	push 24
	jmp service_interrupt
		
global isr25
isr25:
	push 0
	push 25
	jmp service_interrupt
		
global isr26
isr26:
	push 0
	push 26
	jmp service_interrupt
		
global isr27
isr27:
	push 0
	push 27
	jmp service_interrupt
		
global isr28
isr28:
	push 0
	push 28
	jmp service_interrupt
		
global isr29
isr29:
	push 0
	push 29
	jmp service_interrupt
		
global isr30
isr30:
	push 30
	jmp service_interrupt
		
global isr31
isr31:
	push 0
	push 31
	jmp service_interrupt


; IRQs

EXTERN tasking_handler
EXTERN __esp

global isr32
isr32:

	push ebp
    push edi
    push esi
    push edx
    push ecx
    push ebx
    push eax

    mov ax, ds
    push eax

    push esp
    call tasking_handler
    mov esp, eax

	pop eax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	

    pop eax
    pop ebx
    pop ecx
    pop edx
    pop esi
    pop edi
    pop ebp
    
    iret
		
global isr33
isr33:
	push 0
	push 33
	jmp service_interrupt
		
global isr34
isr34:
	push 0
	push 34
	jmp service_interrupt

global isr35
isr35:
	push 0
	push 35
	jmp service_interrupt

global isr36
isr36:
	push 0
	push 36
	jmp service_interrupt

global isr37
isr37:
	push 0
	push 37
	jmp service_interrupt

global isr38
isr38:
	push 0
	push 38
	jmp service_interrupt

global isr39
isr39:
	push 0
	push 39
	jmp service_interrupt

global isr40
isr40:
	push 0
	push 40
	jmp service_interrupt

global isr41
isr41:
	push 0
	push 41
	jmp service_interrupt

global isr42
isr42:
	push 0
	push 42
	jmp service_interrupt

global isr43
isr43:
	push 0
	push 43
	jmp service_interrupt

global isr44
isr44:
	push 0
	push 44
	jmp service_interrupt

global isr45
isr45:
	push 0
	push 45
	jmp service_interrupt

global isr46
isr46:
	push 0
	push 46
	jmp service_interrupt

global isr47
isr47:
	push 0
	push 47
	jmp service_interrupt

global isr48
isr48:
	push 0
	push 48
	jmp service_interrupt


[EXTERN dispatch_interrupt]

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
