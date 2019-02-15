include ../rules.mk

OUT = vgatty
OBJS = vgatty.o
EXT_OBJS = ../sys/sys.o

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
