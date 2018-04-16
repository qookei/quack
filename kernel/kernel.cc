#include <stddef.h>
#include <stdint.h>
#include <io/serial.h>
#include <io/ports.h>
#include <cpuid.h>
#include <vsprintf.h>
#include <trace/trace.h>
#include <trace/stacktrace.h>
#include <interrupt/idt.h>
#include <interrupt/isr.h>
#include <pic/pic.h>
#include <multiboot.h>
#include <paging/pmm.h>
#include <paging/paging.h>
#include <tty/tty.h>
#include <tty/backends/vesa_text.h>
#include <kheap/liballoc.h>
#include <io/rtc.h>
#include <kbd/ps2kbd.h>
#include <mouse/ps2mouse.h>
#include <tasking/tasking.h>
#include <syscall/syscall.h>
#include <fs/devfs.h>
#include <fs/ustar.h>
#include <devices/devices.h>

void serial_writestr(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		serial_write_byte(data[i]);
}
 
void serial_writestring(const char* data) {
	serial_writestr(data, strlen(data));
}


int kprintf(const char *fmt, ...) {
	char buf[1024];
	va_list va;
	va_start(va, fmt);
	int ret = vsprintf(buf, fmt, va);
	va_end(va);
	serial_writestring(buf);
	return ret;
}

bool axc = false;
char lastkey = '\0';

uint8_t u = 0;

extern uint8_t minute;
extern uint8_t hour;
extern uint8_t second;

extern uint8_t day;
extern uint8_t month;
extern uint32_t year;

const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

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


void pit_freq(uint32_t frequency) {
    uint16_t x = 1193182 / frequency;
    if ((1193182 % frequency) > (frequency / 2))
        x++;
        
    outb(0x40, (uint8_t)(x & 0x00ff));
    outb(0x40, (uint8_t)(x / 0x0100));
}

multiboot_info_t *mbootinfo;
extern void* memcpy(void*, const void*, size_t);

// void fastmemcpy(uint32_t dst, uint32_t src, uint32_t size) {
// 	asm volatile ("mov %0, %%ecx\nmov %1, %%edi\nmov %2, %%esi\nrep movsb" : : "r"(size), "r"(dst), "r"(src) : "%ecx","%edi","%esi");
// }


const char* mem_type_names[] = {"", "Available", "Reserved", "ACPI", "NVS", "Bad RAM"};

void gdt_new_setup(void);

void gdt_set_tss_stack(uint32_t);

void gdt_ltr(void);

const char *get_init_path(char *cmdline) {
	if (strlen(cmdline)) {
		char *where = strstr(cmdline, "init=");

		if (where == NULL)
			return "/bin/init";

		uint32_t len = 0;

		char *tmp = where + 5;

		char *a = strchr(tmp, ',');
		char *b = strchr(tmp, '\0');
		char *c = strchr(tmp, ' ');
		char *d;
		char *e;

		if (a == NULL) e = b;
		if (b == NULL) e = a;
		if (c == NULL) d = e;
		if (d == NULL) 
			d = tmp + strlen(tmp);

		len = (uint32_t)(d - tmp);

		char *path = (char *)kmalloc(len + 1);
		memset(path, 0, len + 1);
		memcpy(path, tmp, len);

		return path;
	} else
		return "/bin/init";
}

uint32_t boot_time;

uint32_t getuptime() {
	return gettime() - boot_time;
}

extern "C" { 


extern uint8_t _rodata;

void kernel_main(multiboot_info_t *d) {


	if(d->framebuffer_width == 80) {
		char *v = (char*)0xC00B8000;
		memset(v, 0, 80 * 25 * 2);
		*v++ = 'U';
		*v++ = 0x07;
		*v++ = 'N';
		*v++ = 0x07;
		*v++ = 'S';
		*v++ = 0x07;
		*v++ = 'U';
		*v++ = 0x07;
		*v++ = 'P';
		*v++ = 0x07;
		*v++ = 'P';
		*v++ = 0x07;
		*v++ = 'O';
		*v++ = 0x07;
		*v++ = 'R';
		*v++ = 0x07;
		*v++ = 'T';
		*v++ = 0x07;
		*v++ = 'E';
		*v++ = 0x07;
		*v++ = 'D';
		*v++ = 0x07;
		*v++ = ' ';
		*v++ = 0x07;
		*v++ = ' ';
		*v++ = 0x07;
		
		return;
	}

	/* Initialize terminal interface */
	serial_init();

	if (((uint32_t)d) - 0xC0000000 > 0x800000) {
		kprintf("[kernel] mboot hdr out of page");
		return;
	}

	gdt_new_setup();
	kprintf("[kernel] GDT ok\n");

	pic_remap(0x20, 0x28);
	kprintf("[kernel] PIC ok\n");

	idt_init();
	kprintf("[kernel] IDT ok\n");
	
	pmm_init(d);

	paging_init();

	vesa_text_tty_set_mboot(d);
	tty_setdev(vesa_text_tty_dev);

	tty_init();

	mbootinfo = d;

	//asm volatile ("sti");
	
	uint32_t brand[12];
	__cpuid(0x80000002 , brand[0], brand[1], brand[2], brand[3]);
	__cpuid(0x80000003 , brand[4], brand[5], brand[6], brand[7]);
	__cpuid(0x80000004 , brand[8], brand[9], brand[10], brand[11]);

	printf("[kernel] cpu brand: %s\n", (const char*)brand);	

	ps2_kbd_init();

	ps2_kbd_reset_buffer();
	
	ps2mouse_init();
	boot_time = gettime();

	// printf("[kernel] params: %s\n", (const char*)(0xC0000000 + d->cmdline));

	printf("free memory: %u out of %u bytes free\n", free_pages() * 4096, max_pages() * 4096);

	// multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)(d->mmap_addr + 0xC0000000);
	// while((uint32_t)mmap < (d->mmap_addr + 0xC0000000 + d->mmap_length)) {
		
	// 	uint32_t addr = (uint32_t)mmap->addr;
	// 	uint32_t len = (uint32_t)mmap->len;

	// 	printf("%08x - %08x | %s\n", addr, addr + len, mem_type_names[mmap->type]);

	// 	mmap = (multiboot_memory_map_t*) ((uint32_t)mmap + mmap->size + sizeof(mmap->size));
	// }
	
	printf("\n");

	for (int i = 0; i < 8; i++) {
		printf("\e[%um", 40 + i);
		printf(" ");
	}
	printf("\e[1m");
	for (int i = 0; i < 8; i++) {
		printf("\e[%um", 40 + i);
		printf(" ");
	}
	printf("\e[0m");
	printf("\e[40m");
	printf("\e[1m");
	printf("\e[39m");
	printf("\e[0m");
	printf("\n");

	syscall_init();

	uint32_t stack;

	pit_freq(4000);


	tasking_init();

	vfs_init();
	devfs_init();

	// init devices
	dev_tty_init();
	dev_initrd_init();
	dev_videomode_init();
	dev_mouse_init();
	dev_uptime_init();

	mount("/dev/initrd", "/", "ustar", 0);

	char *cmdline = (char *)(0xC0000000 + d->cmdline);


	printf("boot time %u\n", boot_time);
	printf("uptime %u\n", getuptime());

	tasking_setup(get_init_path(cmdline));		// default path

	asm volatile ("mov %%esp, %0" : "=r"(stack));

	gdt_set_tss_stack(stack);
	gdt_ltr();

	asm volatile ("sti");
	
	while(true) {
	}
}

}
