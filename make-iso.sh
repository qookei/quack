#!/bin/sh

mkdir -p isodir/boot/grub

cat >isodir/boot/grub/grub.cfg << EOL
menuentry "quack - default" {
	multiboot /boot/quack.elf
	set gfxpayload=text
	boot
}
EOL

cp kernel/quack.elf isodir/boot/quack.elf
grub-mkrescue -o quack.iso isodir > /dev/null
rm -r isodir
