# quack
quack is a microkernel based operating system. It currently supports x86\_64 with a planned i386 port.

## Building

For build instructions, refer to the [bootstrap](https://gitlab.com/quack-os/bootstrap) repository.

## Planned features
- fully working x86\_64 and i386 ports
- a generic device management and resource allocation system
- a [mlibc](https://github.com/managarm/mlibc) port
- device drivers for common devices like ATA hard disks, PS/2 keyboards and mice, USB, some (virtualized) graphics adapters
- an nice and useable user space
- networking

## Reporting issues

### Build issues
When reporting build issues please attach a full log to the issue report along a description of what you were doing, the distro/OS you're on, which commit you tried to compile, and what architecture you were compiling quack for. Please report build issues on the quack/bootstrap repository.

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
