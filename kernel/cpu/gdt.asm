global setup_gdt

gdt_start:
	dd 0x0
	dd 0x0
	
gdt_code:
	dw 0xFFFF
	dw 0x0
	db 0x0
	db 10011010b
	db 11001111b
	db 0x0

gdt_data:
	dw 0xFFFF
	dw 0x0
	db 0x0
	db 10010010b
	db 11001111b
	db 0x0
	
gdt_user_code:
	dw 0xFFFF
	dw 0x0
	db 0x0
	db 11111010b
	db 11001111b
	db 0x0

gdt_user_data:
	dw 0xFFFF
	dw 0x0
	db 0x0
	db 11110010b
	db 11001111b
	db 0x0

gdt_end:

gdt_descriptor:
	dw gdt_end - gdt_start - 1
	dd gdt_start

setup_gdt:
   lgdt [gdt_descriptor]
   jmp   0x08:.reload_cs
.reload_cs:
   mov   ax, 0x10
   mov   ds, ax
   mov   es, ax
   mov   fs, ax
   mov   gs, ax
   mov   ss, ax
   ret