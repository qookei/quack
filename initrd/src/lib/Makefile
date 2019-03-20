include ../rules.mk

OUT = lib.o
OBJS = elf.o uuid.o string.o ctype.o liballoc.o liballoc_funcs.o

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
