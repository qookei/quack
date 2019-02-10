#!/bin/sh

qemu-system-i386 -kernel kernel.elf -initrd initrd/initrd -serial stdio $@
