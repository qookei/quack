project('quack', 'c')

c_args = meson.get_cross_property('c_args')
link_args = meson.get_cross_property('link_args')

arch_srcs = []
arch_inc_dirs = []
arch_dependencies = []
arch_linker_file = ''

c_srcs = files(
  'vsnprintf.c',
  'kmesg.c',
  'util.c',

  'lib/ctype.c',
  'lib/string.c',
  'lib/stdlib.c',

  'mm/liballoc.c',
  'mm/liballoc_funcs.c',

  'panic.c',
  'cmdline.c')

inc_dirs = include_directories(
  '.',
  'lib/')

arch_path = 'arch/@0@/'.format(host_machine.cpu())
subdir(arch_path)

out = executable(
  'quack.elf',
  c_srcs, arch_srcs,
  include_directories : [inc_dirs, arch_inc_dirs],
  dependencies: arch_dependencies,
  c_args: ['-Wall', '-Wextra', c_args],
  link_args: ['-T', arch_linker_file, link_args],
)
