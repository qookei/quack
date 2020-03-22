project('duck', 'c', 'cpp', default_options: ['cpp_std=c++17', 'c_std=c18'])

in_kernel = get_option('in_kernel')
do_headers = get_option('do_headers')
do_sources = get_option('do_sources')

duck_includes = include_directories('inc')

if in_kernel
	# the kernel does not need the library sources, and doesn't use
	# the sysroot's /usr/include so we just declare a dependency
	# with the includes
	duck_dep = declare_dependency(include_directories: duck_includes)
else
	if do_headers
		headers = files(
			'inc/duck/error.h',
			'inc/duck/calls.h',
			'inc/duck/types.h',
			'inc/duck/syscall.h')
		install_headers(headers, subdir: 'duck')
	elif do_sources
		sources = files('src/stubs.cpp')
		libduck = static_library('duck', sources,
				include_directories: duck_includes,
				pic: false, install: true)

		pkg = import('pkgconfig')
		pkg.generate(libduck)
	endif
endif
