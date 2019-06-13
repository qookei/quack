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
menuentry "quack - default" {
	multiboot /boot/quack.elf
	set gfxpayload=text
	boot
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
