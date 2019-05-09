#CC = clang --target=i686-pc-elf -march=znver1
CC = i686-elf-gcc
ASM = nasm

CFLAGS = -ffreestanding -O$(OLEVEL) -Wall -Wextra -std=gnu11 -Ikernel -Ikernel/lib -fno-omit-frame-pointer -mno-sse -mno-avx -nostdlib -g
ARCH_OBJ = i386.o
LINKER_SCRIPT = linker.ld
