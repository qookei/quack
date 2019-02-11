#!/bin/sh

mkdir -p isodir/boot/grub

cat >isodir/boot/grub/grub.cfg << EOL
menuentry "quack - default" {
	multiboot /boot/kernel.elf
	module /boot/initrd
	set gfxpayload=80x25
	boot
}

menuentry "quack - custom resolution" {
	multiboot /boot/kernel.elf
	module /boot/initrd
	echo "Enter resolution:"
	read gfxpayload
	boot
}
EOL

cp kernel.elf isodir/boot/kernel.elf
make -C initrd
cp initrd/initrd isodir/boot/initrd
grub-mkrescue -o quack.iso isodir > /dev/null 2>&1
rm -r isodir
