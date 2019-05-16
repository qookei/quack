ENTRY(_loader)
OUTPUT_FORMAT(elf64-x86-64)

KERNEL_LMA = 0x0000000000200000;
KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS {
	. = KERNEL_LMA + KERNEL_VMA;

	.text : AT(ADDR(.text) - KERNEL_VMA) {
		*(.multiboot)
		*(.text)
		_rodata = .;
		*(.rodata*)
	}

	.data ALIGN (0x1000) : AT(ADDR(.data) - KERNEL_VMA) {
		*(.data)
	}

	.bss : AT(ADDR(.bss) - KERNEL_VMA) {
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}
}
