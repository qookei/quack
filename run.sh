#!/bin/sh

qemu-system-i386 -kernel quack.elf -initrd initrd/initrd -debugcon stdio $@
