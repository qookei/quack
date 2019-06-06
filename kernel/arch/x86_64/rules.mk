CC = x86_64-elf-gcc
ASM = nasm

CFLAGS = -ffreestanding -O$(OLEVEL) -Wall -Wextra -std=gnu11 -Ikernel -Ikernel/lib -fno-omit-frame-pointer -mno-sse -mno-avx -nostdlib -DARCH=x86_64 -mcmodel=kernel -z max-page-size=0x1000 -DNO_DEBUG_OUT
ARCH_OBJ = x86_64.o
LINKER_SCRIPT = linker.ld
