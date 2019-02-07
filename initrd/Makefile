SRC = src/init.c
OUT = $(SRC:src/%.c=out/%)
TAR_OBJ = $(OUT:out/%=%)
OUTDIR = out

initrd: $(OUT) $(OUTDIR)
	@printf "ARCHIVE\t\t%s\n" $@
	-@rm -f initrd > /dev/null 2>&1
	@tar cf initrd -C $(OUTDIR)/ $(TAR_OBJ)

out/%: src/%.c $(OUTDIR)
	@printf "CC\t\t%s\n" $@
	@i686-elf-gcc $< src/sys/syscall.c -o $@ -mno-sse -mno-avx -ffreestanding -nostdlib -O0 -std=gnu11

$(OUTDIR):
	@printf "MKDIR\t\t%s\n" $@
	@mkdir $(OUTDIR)
