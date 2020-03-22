project('quack', 'c', 'cpp', default_options: ['cpp_std=c++17'])

c_args = ['-ffreestanding', '-Wall', '-Wextra']
link_args = ['-ffreestanding', '-nostdlib']
cpp_args = ['-fno-rtti', '-fno-exceptions', '-fno-use-cxa-atexit', '-fno-threadsafe-statics', '-DCXXSHIM_INTEGRATE_GCC', '-DDUCK_IN_KERNEL']

if meson.is_cross_build()
	c_args += [meson.get_cross_property('c_args')]
	link_args += meson.get_cross_property('link_args')
endif

arch_srcs = []
arch_inc_dirs = []
arch_dependencies = []
arch_linker_file = ''

cxx_srcs = files(
  'lib/vsnprintf.cpp',
  'lib/kmesg.cpp',
  'util/util.cpp',
  'util/initrd.cpp',
  'util/spinlock.cpp',

  'lib/ctype.cpp',
  'lib/string.cpp',
  'lib/stdlib.cpp',

  'mm/heap.cpp',
  'mm/vm.cpp',

  'lib/panic.cpp',
  'util/cmdline.cpp',

  'genfb/genfb.cpp',

  'proc/scheduler.cpp',
  'loader/elf64.cpp',

  'generic_irq.cpp',
  'syscall.cpp',

  'ubsan.cpp',

  'symtab/symtab.cpp',

  'object/group.cpp',
  'object/ipc_queue.cpp',

  'kernel.cpp')

inc_dirs = include_directories(
  '.',
  'lib/',
  'util/')

arch_path = 'arch/@0@/'.format(host_machine.cpu())
subdir(arch_path)

frigg = subproject('frigg', default_options: ['frigg_no_install=true'])
cxxshim = subproject('cxxshim')

frigg_dep = frigg.get_variable('frigg_dep')
cxxshim_dep = cxxshim.get_variable('cxxshim_dep')

# Maybe figure a better way to do this
duck_inc = include_directories('../duck/inc/')

out = executable(
  'quack.elf',
  cxx_srcs, arch_srcs,
  include_directories : [inc_dirs, arch_inc_dirs, duck_inc],
  dependencies: [arch_dependencies, frigg_dep, cxxshim_dep],
  c_args: [c_args],
  cpp_args: [cpp_args, c_args],
  link_args: ['-T', arch_linker_file, link_args, '-flto'],
  install: true,
  install_dir: '.'
)
