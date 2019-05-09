include rules.mk

OBJS = boot/boot.o arch_entry.o \
	   cpu/gdt/gdt.o cpu/gdt/gdt_load.o \
	   cpu/int/idt.o cpu/int/interrupt.o cpu/int/idt_load.o cpu/int/isr.o \
	   mm/pmm.o mm/vmm.o

OWN_CFLAGS = -I../.. -I.. -I../../lib -I.

$(ARCH_OBJ): $(OBJS)
	@printf "\tARCH LINK\t\t%s\n" $@
	$(CC) $^ -r -o $@ $(CFLAGS) $(OWN_CFLAGS)

%.o: %.c
	@printf "\tARCH CC\t\t%s\n" $@
	@$(CC) -c $< -o $@ $(CFLAGS) $(OWN_CFLAGS)

%.o: %.asm
	@printf "\tARCH ASM\t\t%s\n" $@
	@$(ASM) -felf32 -F dwarf $< -o $@ $(ASMFLAGS)

.PHONY: clean

clean:
	@printf "\tARCH cleaning\n"
	@-rm $(ARCH_OBJ)
	@-rm $(OBJS)
