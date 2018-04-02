global _loader
extern kernel_main
 
MODULEALIGN equ  1<<0
MEMINFO     equ  1<<1
VIDEO       equ  1<<2
FLAGS       equ  MODULEALIGN | MEMINFO | VIDEO
MAGIC       equ    0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    
    ; keep it all 0 since this is not A.OUT

    dd 0
    dd 0
    dd 0
    dd 0
    dd 0

    ; video mode

    dd 0
    dd 0
    dd 0
    dd 0


 
KERNEL_VIRTUAL_BASE equ 0xC0000000
KERNEL_PAGE_NUMBER equ (KERNEL_VIRTUAL_BASE >> 22)
 
 
section .data
align 0x1000
page_dir:
    dd 0x00000083
    dd 0x00400083
    dd 0x00800083
    times (KERNEL_PAGE_NUMBER - 3) dd 0                 ; Pages before kernel space.
    dd 0x00000083
    dd 0x00400083
    dd 0x00800083
    times (1024 - KERNEL_PAGE_NUMBER - 3) dd 0          ; Pages after the kernel image.
 
 
 
section .text
align 4

STACKSIZE equ 0x4000
 
loader equ (_loader - 0xC0000000)
global loader
 
_loader:
    mov ecx, (page_dir - KERNEL_VIRTUAL_BASE)
    mov cr3, ecx
 
    mov ecx, cr4
    or ecx, 0x00000010
    mov cr4, ecx
 
    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx
 
    lea ecx, [higher_half]
    jmp ecx
 
higher_half:
    mov dword [page_dir], 0
    mov dword [page_dir + 4], 0
    invlpg [0]
    invlpg [0x40000]
 
    mov esp, stack+STACKSIZE
    
    add ebx, 0xC0000000
    push ebx
 
    call  kernel_main
l:    
    hlt
    jmp l
 
 
section .bss
align 32
stack:
    resb STACKSIZE      ; reserve 16k stack on a uint64_t boundary