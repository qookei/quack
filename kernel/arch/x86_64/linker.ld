ENTRY(_loader)
OUTPUT_FORMAT(elf64-x86-64)

KERNEL_LMA = 0x0000000000200000;
KERNEL_SMP = 0x0000000000400000;
KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS {
	PROVIDE (__executable_start = KERNEL_VMA + KERNEL_LMA);
	. = KERNEL_VMA + KERNEL_LMA + SIZEOF_HEADERS;

	.text : AT(ADDR(.text) - KERNEL_VMA) {
		*(.multiboot)
		*(.text)
		*(.rodata)
	}

	.data ALIGN (0x1000) : AT(ADDR(.data) - KERNEL_VMA) {
		*(.data)
	}

	.init_array : {
		_init_array_begin = .;
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array*))
		_init_array_end = .;
	}

	.bss : AT(ADDR(.bss) - KERNEL_VMA) {
		*(COMMON)
		*(.bss)
	}

	_trampoline_start_virt = .;
	. = 0x1000;

	.trampoline : AT(KERNEL_SMP) {
		_trampoline_start = .;
		*(.trampoline)
		_trampoline_end = .;
	}
}
