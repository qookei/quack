include rules.mk

OBJS = boot/boot.o arch_entry.o

OWN_CFLAGS = -I../.. -I.. -I../../lib -I.

$(ARCH_OBJ): $(OBJS)
	@printf "\tARCH LINK\t\t%s\n" $@
	$(CC) -r $^ -o $@ $(CFLAGS) $(OWN_CFLAGS)

%.o: %.c
	@printf "\tARCH CC\t\t%s\n" $@
	@$(CC) -c $< -o $@ $(CFLAGS) $(OWN_CFLAGS)

%.o: %.asm
	@printf "\tARCH ASM\t\t%s\n" $@
	@$(ASM) -felf64 -F dwarf $< -o $@ $(ASMFLAGS)

.PHONY: clean

clean:
	@printf "\tARCH cleaning\n"
	@-rm $(ARCH_OBJ)
	@-rm $(OBJS)
