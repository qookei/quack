include ../rules.mk

OUT = sys.o
OBJS = syscall.o elf.o

$(OUT): $(OBJS)
	@printf "LINK\t\t%s\n" $@
	@$(LD) -r $^ -o $@

%.o: %.c
	@printf "CC\t\t%s\n" $@
	@$(CC) -c $< -o $@ $(CFLAGS)

.PHONY: clean

clean:
	-rm $(OBJS)
	-rm $(OUT)
