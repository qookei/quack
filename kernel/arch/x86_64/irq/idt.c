#include "idt.h"

isr_fn_t handlers[IDT_ENTRIES] = {
	isr0,   isr1,   isr2,   isr3,   isr4,   isr5,   isr6,   isr7,
	isr8,   isr9,   isr10,  isr11,  isr12,  isr13,  isr14,  isr15,
	isr16,  isr17,  isr18,  isr19,  isr20,  isr21,  isr22,  isr23,
	isr24,  isr25,  isr26,  isr27,  isr28,  isr29,  isr30,  isr31,
	isr32,  isr33,  isr34,  isr35,  isr36,  isr37,  isr38,  isr39,
	isr40,  isr41,  isr42,  isr43,  isr44,  isr45,  isr46,  isr47,
	isr48,  isr49,  isr50,  isr51,  isr52,  isr53,  isr54,  isr55,
	isr56,  isr57,  isr58,  isr59,  isr60,  isr61,  isr62,  isr63,
	isr64,  isr65,  isr66,  isr67,  isr68,  isr69,  isr70,  isr71,
	isr72,  isr73,  isr74,  isr75,  isr76,  isr77,  isr78,  isr79,
	isr80,  isr81,  isr82,  isr83,  isr84,  isr85,  isr86,  isr87,
	isr88,  isr89,  isr90,  isr91,  isr92,  isr93,  isr94,  isr95,
	isr96,  isr97,  isr98,  isr99,  isr100, isr101, isr102, isr103,
	isr104, isr105, isr106, isr107, isr108, isr109, isr110, isr111,
	isr112, isr113, isr114, isr115, isr116, isr117, isr118, isr119,
	isr120, isr121, isr122, isr123, isr124, isr125, isr126, isr127,
	isr128, isr129, isr130, isr131, isr132, isr133, isr134, isr135,
	isr136, isr137, isr138, isr139, isr140, isr141, isr142, isr143,
	isr144, isr145, isr146, isr147, isr148, isr149, isr150, isr151,
	isr152, isr153, isr154, isr155, isr156, isr157, isr158, isr159,
	isr160, isr161, isr162, isr163, isr164, isr165, isr166, isr167,
	isr168, isr169, isr170, isr171, isr172, isr173, isr174, isr175,
	isr176, isr177, isr178, isr179, isr180, isr181, isr182, isr183,
	isr184, isr185, isr186, isr187, isr188, isr189, isr190, isr191,
	isr192, isr193, isr194, isr195, isr196, isr197, isr198, isr199,
	isr200, isr201, isr202, isr203, isr204, isr205, isr206, isr207,
	isr208, isr209, isr210, isr211, isr212, isr213, isr214, isr215,
	isr216, isr217, isr218, isr219, isr220, isr221, isr222, isr223,
	isr224, isr225, isr226, isr227, isr228, isr229, isr230, isr231,
	isr232, isr233, isr234, isr235, isr236, isr237, isr238, isr239,
	isr240, isr241, isr242, isr243, isr244, isr245, isr246, isr247,
	isr248, isr249, isr250, isr251, isr252, isr253, isr254, isr255,
};

idt_entry_t idt_entries[IDT_ENTRIES];

void idt_set_gate(uint8_t idx, uintptr_t offset, uint16_t selector, uint8_t flags) {
	idt_entries[idx].offset_low  = offset & 0xFFFF;
	idt_entries[idx].offset_mid  = (offset >> 16) & 0xFFFF;
	idt_entries[idx].offset_high = (offset >> 32) & 0xFFFFFFFF;

	idt_entries[idx].selector = selector;
	idt_entries[idx].attrib = flags;
	
	idt_entries[idx].zero_1 = 0;
	idt_entries[idx].zero_2 = 0;
}

static inline void idt_lidt(void *idtp) {
	asm volatile("lidt (%0)" : : "r"(idtp));
}

idt_ptr_t idtp;

void idt_init(void) {
	for (size_t i = 0; i < IDT_ENTRIES; i++) {
		idt_set_gate(i, (uintptr_t)handlers[i], 0x08, 0x8E);
	}

	idtp.limit = IDT_ENTRIES * sizeof(idt_entry_t) - 1;
	idtp.base = (uint64_t) &idt_entries;

	idt_lidt(&idtp);
}

void idt_just_load(void) {
	idt_lidt(&idtp);
}
