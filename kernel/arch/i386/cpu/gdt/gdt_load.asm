bits 32

global gdt_load

gdtr:
	dw 0
	dd 0

gdt_load:
	mov [gdtr + 2], ecx
	mov [gdtr], dx
	lgdt [gdtr]
	jmp   0x08:.reload_cs
.reload_cs:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	mov ax, 0x28
	ltr ax

	ret
