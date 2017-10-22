global _loader
extern kernel_main
 
MODULEALIGN equ  1<<0
MEMINFO     equ  1<<1
FLAGS       equ  MODULEALIGN | MEMINFO
MAGIC       equ    0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
 
KERNEL_VIRTUAL_BASE equ 0xC0000000
KERNEL_PAGE_NUMBER equ (KERNEL_VIRTUAL_BASE >> 22)
 
 
section .data
align 0x1000
page_dir:
    dd 0x00000083
    times (KERNEL_PAGE_NUMBER - 1) dd 0                 ; Pages before kernel space.
    dd 0x00000083
    times (1024 - KERNEL_PAGE_NUMBER - 1) dd 0  ; Pages after the kernel image.
 
 
 
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
    invlpg [0]
 
    mov esp, stack+STACKSIZE
    ; push eax
 
    ; pass Multiboot info structure -- WARNING: This is a physical address and may not be
    ; in the first 4MB!
    ; push ebx
 
    call  kernel_main
l:    
    hlt
    jmp l
 
 
section .bss
align 32
stack:
    resb STACKSIZE      ; reserve 16k stack on a uint64_t boundary