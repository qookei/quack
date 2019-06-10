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
- `meson`, `ninja`, `python`

To generate a bootable ISO image, you need:
- `grub-mkrescue` and everything else needed to use it(`mkisofs`, `xorriso`, etc.)

### Supported architectures
quack currently supports 2 architectures: i386 and x86\_64. Below is a list of architectures and paths(relative to project root) to their cross files(required for building)
- x86\_64 - `kernel/arch/x86_64-cross-file`
- i386 - not yet made, will be `kernel/arch/i386-cross-file`

### Instructions

#### TL;DR

The whole process looks like this:

```
$ git clone https://gitlab.com/qookei/quack.git
$ cd quack
$ mkdir build
$ cd build
$ meson ../kernel --cross-file <cross file>
$ ninja
```

For explanations, refer to the next section.

#### Step explanations

First, clone the repo and enter the directory:
```
$ git clone https://gitlab.com/qookei/quack.git
$ cd quack
```

To build the kernel, first create a build directory and enter it:
```
$ mkdir build
$ cd build
```

Then, generate the build script with Meson:
```
$ meson ../kernel --cross-file <cross file>
```

Each supported architecture has it's own cross file. Using a specific cross file means building quack for that architecture.
For paths to cross files refer to the list in the section above.

For example, when building the kernel for x86\_64, the `meson` command would look like this:
```
$ meson ../kernel --cross-file ../kernel/arch/x86_64/cross-file.build
```

Afterwards, you're ready to build the kernel using Ninja:
```
$ ninja
```

After that's done, you should have a `quack.elf` file in the build directory.
If you built quack for the i386 or x86\_64 target, you can optionally generate a bootable ISO image using the following command:

```
$ ../make-iso.sh
```

This should produce `quack.iso` in the current directory.
To run this image, preferably use qemu like so:
```
$ qemu-system-x86_64 -cdrom quack.iso -debugcon stdio
```

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
