# quack
quack is a 32bit open source operating system written primarily in C and Assembly.

### Building
To build quack, you need to have the following:
- `i686-elf` GCC and Binutils
- `nasm`
- `python` 2 or 3
- `make`
- `dialog`

To generate a bootable ISO image, you need:
- `grub-mkrescue` and everything else needed to use it(`mkisofs`, `xorriso`, etc.)

Once you acquired all the necessary tools, you are ready to build the OS.
Building is simple and only requires a few steps.
Usually building looks something like this:
```
$ git clone https://gitlab.com/qookei/quack.git
$ cd quack
$ make
```
If you plan on working on the kernel, and the build doesn't need to run on real hardware, use `make DEBUG=1` instead of just `make`.
This will enable the debug port output and debugging symbols. For more information about debug port output, see the comment in `kernel/io/debug_port.c`.

After that's done with no reported errors, you should have a `kernel.bin` file in the project directory. As an optional step, you can build the ISO image with an initrd using the following:
```
$ ./make-iso.sh
```
This should produce `quack.iso` in the current directory.
