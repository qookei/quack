#!/bin/sh

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
	set gfxpayload=auto
}

menuentry "quack - e9 debug out" {
	multiboot /boot/quack.elf debugcon=e9
}

menuentry "quack - vga text debug out" {
	multiboot /boot/quack.elf debugcon=vga
	set gfxpayload=text
}

menuentry "quack - e9 and fb debug out" {
	multiboot /boot/quack.elf debugcon=e9,fb
	set gfxpayload=auto
}

menuentry "quack - e9 and vga text debug out" {
	multiboot /boot/quack.elf debugcon=e9,vga
	set gfxpayload=text
}
EOL

cp quack.elf isodir/boot/quack.elf

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
