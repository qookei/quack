nasm = find_program('nasm')

asm_sources = files(
  'boot/boot.asm',
  'irq/irq_handler.asm',
  'cpu/ctx_switch.asm',
  'io/mem.asm',
  'task/task.asm',
  'cpu/smp_trampoline.asm')

gen = generator(nasm,
  output  : '@BASENAME@.o',
  arguments : ['-f', 'elf64', '@INPUT@', '-o', '@OUTPUT@', '-F', 'dwarf'])

arch_srcs += gen.process(asm_sources)

arch_srcs += files(
  'arch_entry.cpp',

  'cpu/cpu.cpp',
  'cpu/lapic.cpp',
  'cpu/ioapic.cpp',
  'cpu/cpu_data.cpp',
  'cpu/smp.cpp',
  'cpu/lock.cpp',
  'cpu/gdt.cpp',

  'io/port.cpp',
  'io/debug.cpp',
  'io/vga.cpp',
  'io/pci.cpp',

  'irq/idt.cpp',
  'irq/isr.cpp',
  'irq/pic/pic.cpp',

  'task/task.cpp',

  'symtab.cpp',

  'mm/pmm.cpp',
  'mm/vmm.cpp',
  'acpi/lai_host.cpp',
  'acpi/acpi.cpp',
  'acpi/madt.cpp')

arch_inc_dirs += include_directories(
  '.')

add_global_arguments(c_args, language: 'c')
add_global_link_arguments(link_args, language: 'c')

lai = subproject('lai')

arch_dependencies += lai.get_variable('dependency')

arch_linker_file = meson.current_source_dir() + '/linker.ld'
