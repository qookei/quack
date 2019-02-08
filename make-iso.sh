#!/bin/sh

mkdir -p isodir/boot/grub

cat >isodir/boot/grub/grub.cfg << EOL
menuentry "quack - auto" {
	multiboot /boot/kernel.elf
	module /boot/initrd
	set gfxpayload=auto
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
