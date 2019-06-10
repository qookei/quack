nasm = find_program('nasm')

asm_sources = files(
  'boot/boot.asm',
  'irq/irq_handler.asm')

gen = generator(nasm,
  output  : '@BASENAME@.o',
  arguments : ['-f', 'elf64', '@INPUT@', '-o', '@OUTPUT@', '-F', 'dwarf'])

arch_others += gen.process(asm_sources)

arch_c_srcs += files(
  'arch_entry.c',

  'cpu/cpu.c',

  'io/port.c',
  'io/debug.c',
  'io/vga.c',
  'io/pci.c',

  'irq/idt.c',
  'irq/isr.c',
  'irq/pic/pic.c',

  'mm/pmm.c',
  'mm/vmm.c',
  'acpi/lai_host.c',
  'acpi/acpi.c',
  'acpi/madt.c')

arch_inc_dirs += include_directories(
  '.')

add_global_arguments(c_args, language: 'c')
add_global_link_arguments(link_args, language: 'c')

lai = subproject('lai')

arch_dependencies += lai.get_variable('dependency')

arch_linker_file = meson.current_source_dir() + '/linker.ld'
