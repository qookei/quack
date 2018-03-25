global setup_gdt


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