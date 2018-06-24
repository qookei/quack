SRC = uptime.c sh.c pond.c ls.c init.c date.c clear.c
OUT = $(SRC:%.c=%)
OUTDIR = bin

all: $(OUT)
	@rm -f initrd > /dev/null 2>&1
	@tar cf initrd *
	@printf "PACKAGE\t\t%s\n" $@

%: src/%.c $(OUTDIR)
	@i686-elf-gcc $< -o $(OUTDIR)/$@ -ffreestanding -nostdlib -O0
	@printf "CXX\t\t%s\n" $@

$(OUTDIR):
	@mkdir bin
	@printf "MKDIR\t\t%s\n" $@
