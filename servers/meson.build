project('quack-servers', 'c', 'cpp')

sysroot_arg = '--sysroot=@0@'.format(get_option('sysroot'))

duck_dep = dependency('duck')

add_project_arguments([sysroot_arg, meson.get_cross_property('c_args')],
			language: ['cpp', 'c'])

add_project_link_arguments([sysroot_arg, meson.get_cross_property('link_args')],
			language: ['cpp', 'c'])

subdir('startup')
