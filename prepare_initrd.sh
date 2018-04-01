#!/bin/sh

cd initrd

echo "prepare_initrd - Compiling sh"
i686-elf-gcc src/sh.c -o bin/sh -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling init"
i686-elf-gcc src/init.c -o bin/init -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling paint"
i686-elf-gcc src/paint.c -o bin/paint -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling note"
i686-elf-gcc src/note.c -o bin/note -ffreestanding -nostdlib -O0

echo "prepare_initrd - Packaging"
tar cf initrd *
cd ..