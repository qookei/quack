#!/bin/bash

if [[ $1 == "help" ]];
then
	echo "quack make-iso.sh"
	echo "supported arguments:"
	echo -e "\thelp - you are here"
	echo -e "\ttiny - produces a minimal disk image(about 2x size reduction)"
	exit 0
fi

mkdir -p isodir/boot/grub

cat >isodir/boot/grub/grub.cfg << EOL
menuentry "quack - fb debug out" {
	multiboot /boot/quack.elf debugcon=fb
	module /boot/initramfs.tar
	set gfxpayload=auto
}

menuentry "quack - e9 debug out" {
	multiboot /boot/quack.elf debugcon=e9
	module /boot/initramfs.tar
}

menuentry "quack - vga text debug out" {
	multiboot /boot/quack.elf debugcon=vga
	module /boot/initramfs.tar
	set gfxpayload=text
}

menuentry "quack - e9 and fb debug out" {
	multiboot /boot/quack.elf debugcon=e9,fb
	module /boot/initramfs.tar
	set gfxpayload=auto
}

menuentry "quack - e9 and vga text debug out" {
	multiboot /boot/quack.elf debugcon=e9,vga
	module /boot/initramfs.tar
	set gfxpayload=text
}
EOL

cp quack.elf isodir/boot/quack.elf
cp initramfs.tar isodir/boot/initramfs.tar

compress=0
if [[ $1 == "tiny" ]];
then
	compress=1
fi

if [[ $compress == 1 ]];
then
	grub-mkrescue --compress=xz --install-modules="part_msdos iso9660 multiboot configfile" -o quack.iso isodir > /dev/null
else
	grub-mkrescue -o quack.iso isodir > /dev/null
fi

rm -r isodir
