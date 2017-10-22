#include "trace.h"

trace_elem trace_elems[] = {
	{"dispatch_interrupt",0xc0100e97, 0x00000174},
	{"idt_load",0xc0100e50, 0x00000000},
	{"isr0",0xc01011b0, 0x00000000},
	{"isr1",0xc01011b9, 0x00000000},
	{"isr10",0xc0101208, 0x00000000},
	{"isr11",0xc010120f, 0x00000000},
	{"isr12",0xc0101216, 0x00000000},
	{"isr13",0xc010121d, 0x00000000},
	{"isr14",0xc0101224, 0x00000000},
	{"isr15",0xc010122d, 0x00000000},
	{"isr16",0xc0101236, 0x00000000},
	{"isr17",0xc010123f, 0x00000000},
	{"isr18",0xc0101248, 0x00000000},
	{"isr19",0xc0101251, 0x00000000},
	{"isr2",0xc01011c2, 0x00000000},
	{"isr20",0xc010125a, 0x00000000},
	{"isr21",0xc0101263, 0x00000000},
	{"isr22",0xc010126c, 0x00000000},
	{"isr23",0xc0101275, 0x00000000},
	{"isr24",0xc010127e, 0x00000000},
	{"isr25",0xc0101287, 0x00000000},
	{"isr26",0xc0101290, 0x00000000},
	{"isr27",0xc0101296, 0x00000000},
	{"isr28",0xc010129c, 0x00000000},
	{"isr29",0xc01012a2, 0x00000000},
	{"isr3",0xc01011cb, 0x00000000},
	{"isr30",0xc01012a8, 0x00000000},
	{"isr31",0xc01012ac, 0x00000000},
	{"isr32",0xc01012b2, 0x00000000},
	{"isr33",0xc01012b8, 0x00000000},
	{"isr34",0xc01012be, 0x00000000},
	{"isr35",0xc01012c4, 0x00000000},
	{"isr36",0xc01012ca, 0x00000000},
	{"isr37",0xc01012d0, 0x00000000},
	{"isr38",0xc01012d6, 0x00000000},
	{"isr39",0xc01012dc, 0x00000000},
	{"isr4",0xc01011d4, 0x00000000},
	{"isr40",0xc01012e2, 0x00000000},
	{"isr41",0xc01012e8, 0x00000000},
	{"isr42",0xc01012ee, 0x00000000},
	{"isr43",0xc01012f4, 0x00000000},
	{"isr44",0xc01012fa, 0x00000000},
	{"isr45",0xc0101300, 0x00000000},
	{"isr46",0xc0101306, 0x00000000},
	{"isr47",0xc010130c, 0x00000000},
	{"isr5",0xc01011dd, 0x00000000},
	{"isr6",0xc01011e6, 0x00000000},
	{"isr7",0xc01011ef, 0x00000000},
	{"isr8",0xc01011f8, 0x00000000},
	{"isr9",0xc01011ff, 0x00000000},
	{"kernel_main",0xc0100424, 0x00000073},
	{"loader",0x100100010, 0x00000000},
	{"_loader",0xc0100010, 0x00000000},
	{"setup_gdt",0xc010069e, 0x00000000},
	{"serial_init",0xc0100497, 0x0000008d},
	{"stack_trace",0xc0100db3, 0x0000009b},
	{"terminal_write",0xc010027e, 0x00000035},
	{"serial_writestr",0xc01002d9, 0x00000038},
	{"serial_read_byte",0xc0100524, 0x00000039},
	{"terminal_putchar",0xc01001c0, 0x000000be},
	{"serial_write_byte",0xc010055d, 0x00000045},
	{"terminal_setcolor",0xc0100153, 0x00000018},
	{"find_correct_trace",0xc0100d59, 0x0000005a},
	{"serial_writestring",0xc0100311, 0x00000026},
	{"terminal_initialize",0xc0100086, 0x000000cd},
	{"terminal_putentryat",0xc010016b, 0x00000055},
	{"a",0xc01003f7, 0x0000002d},
	{"terminal_writestring",0xc01002b3, 0x00000026},
	{"register_interrupt_handler",0xc010100b, 0x00000066},
	{"unregister_interrupt_handler",0xc0101071, 0x00000068},
	{"inb",0xc01005c1, 0x0000001d},
	{"inl",0xc0100636, 0x0000001c},
	{"inw",0xc01005ff, 0x0000001f},
	{"outb",0xc01005a2, 0x0000001f},
	{"outl",0xc010061e, 0x00000018},
	{"outw",0xc01005de, 0x00000021},
	{"printf",0xc0100337, 0x00000060},
	{"strlen",0xc01006bb, 0x00000027},
	{"io_wait",0xc0100652, 0x00000015},
	{"kprintf",0xc0100397, 0x00000060},
	{"pic_eoi",0xc0100e58, 0x0000003f},
	{"sprintf",0xc0100d33, 0x00000026},
	{"idt_init",0xc0101141, 0x0000006e},
	{"vsprintf",0xc010098f, 0x000003a4},
	{"pic_remap",0xc0101344, 0x00000109},
};

uint32_t ntrace_elems = 85;