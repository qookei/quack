#include <stddef.h>
#include <stdint.h>
#include "io/serial.h"
#include "io/ports.h"
#include <cpuid.h>
#include "vsprintf.h"
#include "trace/trace.h"
#include "trace/stacktrace.h"
#include "interrupt/idt.h"
#include "interrupt/isr.h"
#include "pic/pic.h"
#include "multiboot.h"
#include "paging/pmm.h"
#include "paging/paging.h"
#include "tty/tty.h"
#include "tty/backends/vga_text.h"
#include "tty/backends/vesa_text.h"
#include "kheap/heap.h"
#include "io/rtc.h"
#include "kbd/ps2kbd.h"

void serial_writestr(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		serial_write_byte(data[i]);
}
 
void serial_writestring(const char* data) {
	serial_writestr(data, strlen(data));
}


int printf(const char *fmt, ...) {
	char buf[1024] = {0};
	va_list va;
	va_start(va, fmt);
	int ret = vsprintf(buf, fmt, va);
	va_end(va);
	tty_putstr(buf);
	serial_writestring(buf);
	return ret;
}

int kprintf(const char *fmt, ...) {
	char buf[1024] = {0};
	va_list va;
	va_start(va, fmt);
	int ret = vsprintf(buf, fmt, va);
	va_end(va);
	serial_writestring(buf);
	return ret;
}

bool axc = false;
char lastkey = '\0';

// extern void map_page(void*,void*,int);

bool __attribute__((noreturn)) page_fault(interrupt_cpu_state *state) {
	uint32_t fault_addr;
   	asm volatile("mov %%cr2, %0" : "=r" (fault_addr));

	// The error code gives us details of what happened.
	int present   = !(state->err_code & 0x1); // Page not present
	int rw = state->err_code & 0x2;           // Write operation?
	int us = state->err_code & 0x4;           // Processor was in user-mode?
	int reserved = state->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
	int id = state->err_code & 0x10;          // Caused by an instruction fetch?

	// Output an error message.
	tty_putstr("Page fault (");
	if (present) {tty_putstr("present");}
	else {tty_putstr("not present");}
	if (rw) {tty_putstr(", write");}
	else {tty_putstr(", read");}
	if (us) {tty_putstr(", user-mode");}
	else {tty_putstr(", kernel-mode");}
	if (id) {tty_putstr(", instruction-fetch");}
	if (reserved) {tty_putstr(", reserved");}
	kprintf(") at 0x%08x\n", fault_addr);
	stack_trace(20, 0);
	asm volatile ("1:\nhlt\njmp 1b");
}

uint32_t k = 0;
uint8_t u = 0;

extern uint8_t minute;
extern uint8_t hour;
extern uint8_t second;

extern uint8_t day;
extern uint8_t month;
extern uint32_t year;

const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

bool timer(interrupt_cpu_state *state) {
	read_rtc();
	printf("%c[s", 0x1B);
	printf("%c[%u;%uH", 0x1B, 1, 1);
	printf("%c[%uG", 0x1B, 105);
	printf("%02u %s. %04u [%02u:%02u:%02u]", day, months[month-1], year, hour, minute, second);
	if (!u) k++;
	printf("%c[u", 0x1B);
	u++;
	return true;
}

void mem_dump(void *data, size_t nbytes, size_t bytes_per_line) {
	uint8_t *mem = (uint8_t *)data;
	for (size_t y = 0; y < nbytes; y ++) {
		if (y == 0 || y % bytes_per_line == 0) {
			if (y % bytes_per_line == 0 && y) {
				printf("%c[%uG", 0x1B, 10 + bytes_per_line * 3);
				printf("| ");
				for (size_t i = 0; i < bytes_per_line; i++) {
					char c = mem[y - bytes_per_line + i];
					printf("%c", c >= ' ' && c <= '~' ? c : '.');
				}
				printf("\n");
			}
			printf("%08x: ", (uint32_t)data + y);

		}

		printf("%02x ", mem[y]);

	}

	printf("%c[%uG", 0x1B, 10 + bytes_per_line * 3);
	printf("| ");

	size_t x = (nbytes / bytes_per_line) * bytes_per_line;
	if (x == nbytes) {
		x = nbytes - bytes_per_line;
	}

	for (size_t y = x; y < nbytes; y++) {
		char c = mem[y];
		printf("%c", c >= ' ' && c <= '~' ? c : '.');
	}
	
	printf("\n");
}

const char lower_normal[] = { '\0', '?', '1', '2', '3', '4', '5', '6',     
		'7', '8', '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 
				'u', 'i', 'o', 'p', '[', ']', '\n', '\0', 'a', 's', 'd', 'f', 'g', 
				'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v', 
				'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0', ' '};

bool a(interrupt_cpu_state *state) {

	uint8_t c = inb(0x60);
	
	if (c < 0x57)
		printf("%c", lower_normal[c]);

	return true;

}


void fastmemcpy(uint32_t dst, uint32_t src, uint32_t size) {
	asm volatile ("mov %0, %%ecx\nmov %1, %%edi\nmov %2, %%esi\nrep movsb" : : "r"(size), "r"(dst), "r"(src) : "%ecx","%edi","%esi");
}

const char* mem_type_names[] = {"", "Available", "Reserved", "ACPI", "NVS", "Bad RAM"};

extern "C" { 

void setup_gdt(void);

extern uint8_t _rodata;

void kernel_main(multiboot_info_t *d) {
	/* Initialize terminal interface */

	serial_init();



	setup_gdt();
	kprintf("[kernel] GDT ok\n");

	pic_remap(0x20, 0x28);
	kprintf("[kernel] PIC ok\n");

	idt_init();
	asm volatile ("sti");
	kprintf("[kernel] IDT ok\n");

	register_interrupt_handler(14, page_fault);

	pmm_init(d);

	paging_init();

	heap_init();

	if (d->framebuffer_addr == 0xB8000 || d->framebuffer_addr == 0x0) {
		tty_setdev(vga_text_tty_dev); // vesa
	} else {
		vesa_text_tty_set_mboot(d);
		tty_setdev(vesa_text_tty_dev);
		// 
	}

	tty_init();

	if (((uint32_t)d) - 0xC0000000 > 0x800000) {
		kprintf("[kernel] mboot hdr out of page");
		return;
	}

	

	
	uint32_t brand[12];
	__cpuid(0x80000002 , brand[0], brand[1], brand[2], brand[3]);
	__cpuid(0x80000003 , brand[4], brand[5], brand[6], brand[7]);
	__cpuid(0x80000004 , brand[8], brand[9], brand[10], brand[11]);

	printf("[kernel] cpu brand: %s\n", (const char*)brand);	
	//kprintf("Hello world! CPU brand: %s\n", (const char*)brand);

	ps2_kbd_init();

	// register_interrupt_handler(0x21, a);
	
	printf("[kernel] params: %s\n", (const char*)(0xC0000000 + d->cmdline));
	printf("[kernel] fbuf: 0x%x\n", d->framebuffer_addr);

	
	// void *page = (void *)pmm_alloc();
	// map_page(page, (void*)0x0, 0x3);
	// page = (void *)pmm_alloc();
	// map_page(page, (void*)0x1000, 0x3);
	
	// uint8_t *mem = (uint8_t *)kmalloc(128);

	// mem_dump(mem, 128, 16);

	// kfree(mem);

	printf("free pages: %u/%u(%u bytes free)\n", free_pages(), max_pages(), free_pages() * 4096);

	multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)(d->mmap_addr + 0xC0000000);
	while((uint32_t)mmap < (d->mmap_addr + 0xC0000000 + d->mmap_length)) {
		
		uint32_t addr = (uint32_t)mmap->addr;
		uint32_t len = (uint32_t)mmap->len;

		printf("%08x - %08x | %s\n", addr, addr + len, mem_type_names[mmap->type]);

		mmap = (multiboot_memory_map_t*) ((uint32_t)mmap + mmap->size + sizeof(mmap->size));
	}

	printf("\n");

	uint32_t k = 0;

	register_interrupt_handler(0x20, timer);


printf("                 `:+sdmNNmdyo:                                                                        \n");
printf("               .smmNNNMMMMMMMNmo`                                                                     \n");
printf("              /hhdddNMMMMMMMMMMMNy-                                                                   \n");
printf("             :oohhdNMMNNMMMNMNMMMMNo                                                                  \n");
printf("            .osddmmNNNNNNNNNNNNMMMMNh`                                                                \n");
printf("            -+/oNNNNNNNNNNNNNNNMMMMMNy                                                                \n");
printf("            `-/smNNNNMMMMMMMMMMMMMMMMN`                                                               \n");
printf("          `-sy/shdmMMMMMMMMMMMMMMMMMMN/                                                               \n");
printf("         .://+oyhhmNMMMMMMMMMMMMMMMMMN-                                                               \n");
printf("      `.:/+oyhmNmsoshmMMMMMMMMMMMMMMMN.                                                               \n");
printf("   ...:/oydmNh+-     .mMMMMMMMMMMMMMMs                                                                \n");
printf("  /y+shddyo:`    `:o+sNMMMMMMMMMMMMMM`                                                                \n");
printf("   .o+/-      `:odmNNNdmNNmNNNMMMMMMN                                                                 \n");
printf("           `:+shdmmNNMMMMMMmmdydhhmMy                                                                 \n");
printf("         ./o+syhmmNNMMMMMMMMMMMMMNdmyss+++++//:-`                                                     \n");
printf("       ./+osohdmmNNMMMMMMMMMMMNNmmddmddddddhdhddddhyo/:.                                              \n");
printf("     `/ssssyhdmmNmNMMMMMMMMNmmddddmmNNNmys+oosyyysyyyyydmmdy/.                                        \n");
printf("    `:ossoshdmmmmmmNMMMMNNmddddmmmNNmh+//oo++ossossssoooo+oymNNmhso:.                                 \n");
printf("    /ooo/+syddddhhddmmmdddddmmmNNNmh/--:://++o++++oooossso/::/sdNMMMMNh+-                             \n");
printf("   :yso+/ossyyhhhdddddyyosyhdmNNmdh+-:/+++oossooosoooooooooo+/::/ohmMMMMMms-                          \n");
printf("   sddyosoyysyhhhdmmdhssyhhdddho+/::--::/+oyhhhhhhyhyyyyssssssssoo+/+ymMMMMMh+-                       \n");
printf("   hddyyhhhdmmmNNmNNNdsyydddds:-..-....-/+++oydmmmmmmmddhhhhhhhhhyyyso++shNMMMMNy/`                   \n");
printf("   hddyydmNNNNNNMMMMMNmddddhs+:----:::-:///::::/oydmNNNNNNNmmmddddddhhyyo+/oymMMMMMh/                 \n");
printf("   +mmhdmNNNNMMMMMMMMNNmmddhyo:----://:://::://///+oshmMMMMMNNNNmmmmddhhhhyso::odNMMMNs.              \n");
printf("   `hNdmmNNNMMMMMMMMMMNNNNNdm+:::-:/::::::::::://++++ooshdNMMMMMMNNNNNmmmddyyso+//ohNMMN+             \n");
printf("    .mNmmNNMMMMMMMMMMMMMMMMNdooo+///+/::/::////++o++o+o++osydNMMMMMMMMMNNNmdhyysoo++/+ymMyss+//.      \n");
printf("     .dmNmNNMMMMMMMMMMMMMMNdsoooooooo+///////+++++++++++ooosssydNMMMMMMMMNNNmmdhhyysoo+/+ssohm-mh     \n");
printf("       +mNNMMMMMMMMMMMMMMmhssooossssso+++++++++++oooo++oooossssssyhdNMMMMMMMNNNmmddhyysso+/:+oshd     \n");
printf("        `omNNMMMMMMMMMMMNysosssyyssossooooooossossosoooooosyyyyyyyyyyydNMMMMMMMNNNmmddhyysoshddds+:   \n");
printf("           :smMNMMMMMNNNmyysssoossooosososssssyyysssssssyyyhhhhhhhhhhhhhdNMMMMMMMNNmmddhssoo++++/:-`` \n");
printf("              :smNNNNNNNmdhhhysssssossssssyhhhhhyyysssyyhhhhhhhhhdhhhhhhhdddddddddyshMMMNmmmy+.`      \n");
printf("                 -+hNNmmdddhyhyysyssyyyyyyyhhyhyhhhhhhhhhddddddddddddddddddhhyyyyyo++dMMNh+.          \n");
printf("                    `:ohhhyyyyyysyyyyyhhhhhhhhhhhhhhhddddddddddddddddddddddhyyyyhyssoso:              \n");
printf("                        -/syyyyyyhhhhhyhhhhhhhhdhhhhhddddddddddddddmmmmmdhhhyyyhhhys/`                \n");
printf("                           ./oyyhhhhhhhhdddddddddddddddddmmmmmmmmmmmmmdhhhhhhhhhhhs-                  \n");
printf("                               .-/oyhhhhhdhdddddddddmmmmmmmmmmmmmmmmdddddddhhyo+/-                    \n");
printf("                                     `.://hdmysssysdmmmNNmmNNNmmmmhys+/:.`                            \n");
printf("                                         :hmh`   shdm:  ``..---.`                                     \n");
printf("                                        .ydm:    :odh                                                 \n");
printf("                                       `shdm`   `+hm:                                                 \n");
printf("                                       /hddh`   :ydy                                                  \n");
printf("                         /::::::::://++sshmoo. -ydd-                                                  \n");
printf("                      -+oossssyyyyyyysssys. `-:+hmd`                                                  \n");
printf("                          ``..--/syooo++-...../sdmdh`                                                 \n");
printf("                           `-+/++ooooosooooosyyys: `                                                  \n");
printf("                                -yo++/////:--`                                                        \n");

printf("%c[51;3H", 0x1B);
printf("OS: quack");
printf("%c[51;4H", 0x1B);
printf("Kernel: quack 0.0.1");
printf("%c[51;5H", 0x1B);
printf("Uptime: ?");
printf("%c[51;6H", 0x1B);
printf("Packages: ?");
printf("%c[51;7H", 0x1B);
printf("Shell: ?");
printf("%c[51;8H", 0x1B);
printf("CPU: %s", (const char*)brand);
printf("%c[51;9H", 0x1B);
printf("GPU: ? using VBE");
printf("%c[51;10H", 0x1B);
printf("RAM: %uMiB/%uMiB", free_pages() * 4096 / 1024 / 1024, d->mem_upper / 1024);

	getch();
	printf("%cc", 0x1B);

	while(1) {
		
	}
	
	asm volatile ("int $0x00");

}

}
