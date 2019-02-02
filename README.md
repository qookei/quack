# quack
quack is a 32bit open source operating system written primarily in C and Assembly.

### Building
To build quack, you need to have the following:
- `clang` that supports the i386-pc-none-elf target
- `nasm`
- `python` 2 or 3
- `make`
- `grub-mkrescue` and everything else needed to use it(`mkisofs`, `xorriso`, etc.)
- `dialog` for menuconfig

Once you acquired all the necessary tools, you are ready to build the OS.
Building is simple and only requires a few steps.
Usually building looks something like this:
```
$ git clone https://gitlab.com/qookei/quack.git
$ cd quack
$ make
$ ./make-iso.sh
```
If that finished with no reported errors, you should have `quack.iso` in the project directory. You can also run the OS in qemu after building by running `make run` instead of `make`.

