include ../rules.mk

OUT = i8042d
OBJS = i8042d.o
EXT_OBJS = ../sys/sys.o ../lib/lib.o

$(OUT): $(OBJS) $(EXT_OBJS)
	@printf "LINK\t\t%s\n" $@
	@$(CC) $^ -o $@ $(CFLAGS)

%.o: %.c
	@printf "CC\t\t%s\n" $@
	@$(CC) -c $< -o $@ $(CFLAGS)

.PHONY: clean

clean:
	-rm $(OBJS)
	-rm $(OUT)
