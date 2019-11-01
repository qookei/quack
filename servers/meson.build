project('quack-servers', 'c', 'cpp')

add_project_arguments(meson.get_cross_property('c_args'),
			language: ['cpp', 'c'])

add_project_link_arguments(meson.get_cross_property('link_args'),
			language: ['cpp', 'c'])

subdir('startup')
