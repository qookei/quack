global setup_gdt



; tss:
; 	dd 0x00000000	;link
; 	dd ?			; esp0
; 	dd 0x00000010	; ss0
; 	dd 0x00000000	; esp1
; 	dd 0x00000000	; ss1
; 	dd 0x00000000	; esp2
; 	dd 0x00000000	; ss2
; 	dd 0x00000000	; cr3
; 	dd 0x00000000	; eip
; 	dd 0x00000000	; eflags
; 	dd 0x00000000	; eax
; 	dd 0x00000000	; ecx
; 	dd 0x00000000	; edx
; 	dd 0x00000000	; ebx
; 	dd 0x00000000	; esp
; 	dd 0x00000000	; ebp
; 	dd 0x00000000	; esi
; 	dd 0x00000000	; edi
; 	dd 0x00000000	; es
; 	dd 0x00000000	; cs
; 	dd 0x00000000	; ss
; 	dd 0x00000000	; ds
; 	dd 0x00000000	; fs
; 	dd 0x00000000	; gs
; 	dd 0x00000000	; ldtr
; 	dd 0x00000000	; iopb

; gdt_start:
; 	dd 0x0
; 	dd 0x0
	
; gdt_code:
; 	dw 0xFFFF
; 	dw 0x0
; 	db 0x0
; 	db 10011010b
; 	db 11001111b
; 	db 0x0

; gdt_data:
; 	dw 0xFFFF
; 	dw 0x0
; 	db 0x0
; 	db 10010010b
; 	db 11001111b
; 	db 0x0
	
; gdt_user_code:
; 	dw 0xFFFF
; 	dw 0x0
; 	db 0x0
; 	db 11111010b
; 	db 11001111b
; 	db 0x0

; gdt_user_data:
; 	dw 0xFFFF
; 	dw 0x0
; 	db 0x0
; 	db 11110010b
; 	db 11001111b
; 	db 0x0

; tss_gdt_entry:
; 	dw 

; gdt_end:

; gdt_descriptor:
; 	dw gdt_end - gdt_start - 1
; 	dd gdt_start

gdtr:
	DW 0
	DD 0

setup_gdt:
   mov [gdtr + 2], ecx
   mov [gdtr], dx
   lgdt [gdtr]
   jmp   0x08:.reload_cs
.reload_cs:
   mov   ax, 0x10
   mov   ds, ax
   mov   es, ax
   mov   fs, ax
   mov   gs, ax
   mov   ss, ax
   ret