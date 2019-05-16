#!/bin/sh

qemu-system-x86_64 -kernel quack.elf -initrd initrd/initrd -debugcon stdio $@
