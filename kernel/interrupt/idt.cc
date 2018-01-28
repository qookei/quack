#include "idt.h"

isr_f ISRs[] = {
	isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,  isr8,  isr9, 
	isr10, isr11, isr12, isr13, isr14, isr15, isr16, isr17, isr18, isr19, 
	isr20, isr21, isr22, isr23, isr24, isr25, isr26, isr27, isr28, isr29, 
	isr30, isr31, isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39, 
	isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47, isr48,
};

static struct idt_entry {
    uint16_t base_low;
    uint16_t segment_selector;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_high;
} IDT[IDT_size] = {{0,0,0,0,0}};

static struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idtp;

extern "C" {
	extern void idt_load(struct idt_ptr *);
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t segment_selector, uint8_t flags) {
	IDT[num].base_low = base & 0xFFFF;
	IDT[num].base_high = (base >> 16) & 0xFFFF;
	IDT[num].always0 = 0;
	IDT[num].segment_selector = segment_selector;
	IDT[num].flags = flags;
}

void idt_init(void) {
	for (size_t i = 0; i < sizeof(ISRs) / sizeof(isr_f); ++i) {
		idt_set_gate(i, (uint32_t)ISRs[i], 0x08, i==0x30 ? 0xEE : 0x8e);
	}
	
	idtp.limit = (sizeof(struct idt_entry) * IDT_size) - 1;
	idtp.base = (uint32_t) &IDT;
	
	idt_load(&idtp);
}