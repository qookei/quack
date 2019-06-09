# quack
quack is a microkernel based operating system. It currently supports i386 and x86\_64.

## Building

### Dependencies
- for i386:
	- `i686-elf` GCC and Binutils
	- `nasm`
- for x86\_64:
	- `x86_64-elf` GCC and Binutils
	- `nasm`
- `make`

To generate a bootable ISO image, you need:
- `grub-mkrescue` and everything else needed to use it(`mkisofs`, `xorriso`, etc.)

### Instructions
Building is simple and only requires a few steps.

First, clone the repo and enter the directory:
```
$ git clone https://gitlab.com/qookei/quack.git
$ cd quack
```

Then select the architecture by exporting an environment variable. For this example x86\_64 is chosen.
```
$ export ARCH=x86_64
```

To see a list of architectures, either look in the `kernel/arch` directory or run `make` without selecting an architecture, which should look something like this:

```
$ make -C kernel
Makefile:10: *** Please select an architecture, available architectures:  i386  x86_64.  Stop.
```

Afterwards, you're ready to build the kernel.

```
$ make -C kernel
```

The whole process looks like this:

```
$ git clone https://gitlab.com/qookei/quack.git
$ cd quack
$ export ARCH=x86_64
$ make -C kernel
```

After that's done, you should have a `quack.elf` file in the project directory.
If you built quack for the i386 or x86\_64 target, you can optionally generate a bootable ISO image using the following command:

```
$ ./make-iso.sh
```

This should produce `quack.iso` in the current directory.
To run this image, preferably use qemu like so:

```
$ qemu-system-x86_64 -cdrom quack.iso -debugcon stdio
```

Instead of generating an ISO image, you can also directly boot the OS in qemu(i386 only) using:

```
$ ./run.sh
```

You can pass other arguments to this script and they'll be passed directly to qemu.

## Reporting issues

### Build issues
When reporting build issues please attach a full log to the issue report along a description of what you were doing, the distro/OS you're on, which commit you tried to compile, and what architecture you were compiling quack for.

### Kernel panics and userspace issues
When reporting these please include which commit you compiled, the architecture you chose, any reproduction steps you have, and, most importantly, the kernel logs.
Kernel panic logs look something like this:

```
kernel: Kernel panic!
kernel: Message: '...'
...
kernel: halting
```

Userspace issues on the other hand don't have any unified form. They commonly can show as register dumps or assertion failures.

## Planned features
 - fully working x86\_64 and i386 ports
 - a generic device management and resource allocation system
 - a [mlibc](https://github.com/managarm/mlibc) port
 - device drivers for common devices like ATA hard disks, PS/2 keyboards and mice, USB, some (virtualized) graphics adapters
 - an nice and useable user space
 - networking
