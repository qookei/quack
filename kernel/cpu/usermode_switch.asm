; esi - usermode stack
; edi - usermode code

GLOBAL jump_usermode
jump_usermode:
     mov ax,0x23
     mov ds,ax
     mov es,ax 
     mov fs,ax 
     mov gs,ax
 
     push 0x23
     push esi
     pushf
     push 0x1B
     push edi 
     iret