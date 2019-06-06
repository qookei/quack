include rules.mk
include acpi/Makefile

OBJS = boot/boot.o arch_entry.o \
	   io/port.o io/debug.o \
	   irq/idt.o irq/irq_handler.o irq/isr.o irq/pic/pic.o \
	   mm/pmm.o mm/vmm.o \
	   acpi/lai_host.o acpi/acpi.o $(LAI_OBJECTS)

OWN_CFLAGS = -I../.. -I.. -I../../lib -I. -I$(LAI_INCLUDE)

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
