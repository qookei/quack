# quack
quack is a 32bit open source operating system written primarily in C and Assembly.

### Building
To build quack, you need to have the following:
- `clang` that supports the i386-pc-none-elf target
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
After that's done with no reported errors, you should have a `kernel.bin` file in the project directory. As an optional step, you can build the ISO image with an initrd using the following:
```
$ ./make-iso.sh
```
Check the exit code to see if there were any errors during that. 
