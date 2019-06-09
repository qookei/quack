#MAKEFLAGS += -s

ifndef OLEVEL
OLEVEL = 2
endif

ARCHS = $(subst arch/,, $(sort $(patsubst arch/%/, %, $(dir $(wildcard arch/*/)))))

ifndef ARCH
$(error Please select an architecture, available architectures: $(ARCHS))
endif

ARCHDIR = arch/$(ARCH)

include $(ARCHDIR)/rules.mk

ifdef DEBUG
CFLAGS += -g
else
CFLAGS += -DNO_DEBUG_OUT
endif

#OBJS = kernel/kernel.o kernel/io/debug_port.o kernel/io/ports.o kernel/vsprintf.o \
	   kernel/trace/stacktrace.o kernel/kmesg.o kernel/panic.o \
	   kernel/pic/pic.o \
		kernel/lib/stdlib.o kernel/lib/ctype.o kernel/lib/string.o \
	   #kernel/sched/task.o \
	   kernel/sched/task_enter.o kernel/lib/string.o \
	   kernel/lib/stdlib.o kernel/lib/ctype.o kernel/kheap/liballoc.o kernel/kheap/liballoc_funcs.o \
	   kernel/panic.o kernel/kmesg.o kernel/sched/elf.o kernel/initrd.o kernel/sched/sched.o kernel/syscall/handlers.o kernel/syscall/syscall.o

OBJS = vsnprintf.o lib/ctype.o lib/string.o \
	   lib/stdlib.o kmesg.o util.o \
	   mm/liballoc.o mm/liballoc_funcs.o panic.o

LDSCRIPT = $(ARCHDIR)/$(LINKER_SCRIPT)

quack.elf: $(OBJS) $(ARCHDIR)/$(ARCH_OBJ)
	@printf "LINK\t\t%s\n" $@
	@$(CC) -T $(LDSCRIPT) -o quack.elf $^ $(CFLAGS)

%.o: %.c
	@printf "CC\t\t%s\n" $@
	@$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.asm
	@printf "\tARCH ASM\t\t%s\n" $@
	@$(ASM) -felf32 -F dwarf $< -o $@ $(ASMFLAGS)

$(ARCHDIR)/$(ARCH_OBJ): FORCE
	@make -C $(ARCHDIR)

FORCE:

.PHONY: clean

clean:
	@printf "cleaning\n"
	-@rm $(OBJS)
	-@rm quack.elf
	make -C $(ARCHDIR) clean

