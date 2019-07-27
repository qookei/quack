nasm = find_program('nasm')

asm_sources = files(
  'boot/boot.asm',
  'irq/irq_handler.asm',
  'cpu/ctx_switch.asm',
  'cpu/smp_trampoline.asm')

gen = generator(nasm,
  output  : '@BASENAME@.o',
  arguments : ['-f', 'elf64', '@INPUT@', '-o', '@OUTPUT@', '-F', 'dwarf'])

arch_srcs += gen.process(asm_sources)

arch_srcs += files(
  'arch_entry.c',

  'cpu/cpu.c',
  'cpu/lapic.c',
  'cpu/ioapic.c',
  'cpu/cpu_data.c',
  'cpu/smp.c',
  'cpu/lock.c',
  'cpu/gdt.c',

  'io/port.c',
  'io/debug.c',
  'io/vga.c',
  'io/pci.c',
  'io/dev.c',

  'irq/idt.c',
  'irq/isr.c',
  'irq/pic/pic.c',

  'task/task.c',

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
