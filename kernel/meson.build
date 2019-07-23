project('quack', 'c')

c_args = meson.get_cross_property('c_args')
link_args = meson.get_cross_property('link_args')

arch_srcs = []
arch_inc_dirs = []
arch_dependencies = []
arch_linker_file = ''

c_srcs = files(
  'lib/vsnprintf.c',
  'lib/kmesg.c',
  'util/util.c',
  'util/initrd.c',
  'util/spinlock.c',

  'lib/ctype.c',
  'lib/string.c',
  'lib/stdlib.c',

  'mm/heap.c',

  'lib/panic.c',
  'util/cmdline.c',

  'genfb/genfb.c',

  'kobj.c',
  'devmgr.c',

  'kernel.c')

inc_dirs = include_directories(
  '.',
  'lib/',
  'util/')

arch_path = 'arch/@0@/'.format(host_machine.cpu())
subdir(arch_path)

out = executable(
  'quack.elf',
  c_srcs, arch_srcs,
  include_directories : [inc_dirs, arch_inc_dirs],
  dependencies: arch_dependencies,
  c_args: ['-Wall', '-Wextra', c_args],
  link_args: ['-T', arch_linker_file, link_args, '-flto'],
)
