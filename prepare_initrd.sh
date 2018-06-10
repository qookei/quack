#!/bin/sh

cd initrd

if [ ! -d bin ]; then
	echo "prepare_initrd - Creating directory bin"
	mkdir bin
fi

echo "prepare_initrd - Compiling sh"
i686-elf-gcc src/sh.c -o bin/sh -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling init"
i686-elf-gcc src/init.c -o bin/init -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling mouse-cursor"
i686-elf-gcc src/mouse-cursor.c -o bin/mouse-cursor -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling note"
i686-elf-gcc src/note.c -o bin/note -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling dump"
i686-elf-gcc src/dump.c -o bin/dump -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling uptime"
i686-elf-gcc src/uptime.c -o bin/uptime -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling paint"
i686-elf-gcc src/paint.c -o bin/paint -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling crash"
i686-elf-gcc src/crash.c -o bin/crash -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling ls"
i686-elf-gcc src/ls.c -o bin/ls -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling clear"
i686-elf-gcc src/clear.c -o bin/clear -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling pond"
i686-elf-gcc src/pond.c -o bin/pond -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling screenfetch"
i686-elf-gcc src/screenfetch.c -o bin/screenfetch -ffreestanding -nostdlib -O0

echo "prepare_initrd - Compiling sbrk_test"
i686-elf-gcc src/sbrk_test.c -o bin/sbrk_test -ffreestanding -nostdlib -O0

echo "prepare_initrd - Packaging"
tar cf initrd *
cd ..
