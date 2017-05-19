project('molch', 'c', default_options: [
	'c_std=c99',
	'buildtype=release',
	'werror=true',
	'b_lto=true',
	'b_pch=true'
	])
version = '0.1.0'
soversion = '0'

compiler = meson.get_compiler('c')

# specify compiler flags
compiler_flags = []
if (compiler.get_id() == 'clang') or (compiler.get_id() == 'gcc')
	compiler_flags += [
		'-pedantic',
		'-Wall',
		'-Wextra',
		'-Wstrict-prototypes',
		'-Wwrite-strings',
		'-Wshadow',
		'-Winit-self',
		'-Wcast-align',
		'-Wformat=2',
		'-Wmissing-prototypes',
		'-Wstrict-overflow=2',
		'-Wcast-qual',
		'-Wundef',
		'-Wswitch-default',
		'-Wconversion',
		'-fstack-protector-strong',
		'-Wcomma',
		'-Wdouble-promotion',
		'-Wparentheses',
		'-fPIC',
		'-Wunused-macros',
		'-Wmissing-variable-declarations'
	]

	if ['debugoptimized', 'release'].contains(get_option('buildtype'))
		compiler_flags += [
			'-U_FORTIFY_SOURCE',
			'-D_FORTIFY_SOURCE=2'
		]
	endif

	if get_option('enable_asan') or get_option('enable_ubsan')
		if get_option('enable_valgrind') or get_option('enable_bindings')
			error('Can\'t enable sanitizers and valgrind or lua bindings at the same time')
		endif

		optimized_build_options = [
			'release',
			'debugoptimized'
		]
		if not optimized_build_options.contains(get_option('buildtype'))
			error('Buildtype needs to be one of debugoptimized or release')
		endif

		compiler_flags += [
			# sanitizer options
			'-fno-sanitize-recover=all',
			# optimize build
			'-fno-omit-frame-pointer',
			'-fno-common',
			'-fno-optimize-sibling-calls'
		]

		sanitizer_environment = environment()

		if get_option('enable_asan')
			asan = compiler.find_library('asan')
			add_project_link_arguments('-lasan', language: 'c')

			compiler_flags += [
				'-fsanitize=address',
			]

			sanitizer_environment.append('ASAN_OPTIONS', 'detect_stack_use_after_return=1', separator: ':')
			sanitizer_environment.append('ASAN_OPTIONS', 'check_initialization_order=1', separator: ':')
		endif

		if get_option('enable_ubsan')
			ubsan = compiler.find_library('ubsan')
			add_project_link_arguments('-lubsan', language: 'c')

			compiler_flags += [
				'-fsanitize=undefined',
				'-fsanitize=integer',
				'-fsanitize=shift',
				'-fsanitize=integer-divide-by-zero',
				'-fsanitize=unreachable',
				'-fsanitize=vla-bound',
				'-fsanitize=null',
				'-fsanitize=signed-integer-overflow',
				'-fsanitize=bounds',
				'-fsanitize=bounds-strict',
				'-fsanitize=alignment',
				'-fsanitize=object-size',
				'-fsanitize=float-divide-by-zero',
				'-fsanitize=float-cast-overflow',
				'-fsanitize=nonnull-attribute',
				'-fsanitize=returns-nonnull-attribute',
				'-fsanitize-address-use-after-scope',
				'-fsanitize=bool',
				'-fsanitize=enum',
				'-fsanitize=nullability-arg',
				'-fsanitize=nullability-return',
				'-fsanitize=unsigned-integer-overflow'
			]

			sanitizer_environment.append('UBSAN_OPTIONS', 'print_stacktrace=1', separator: ':')
		endif

		add_test_setup('sanitize', env: sanitizer_environment)
	endif
endif

if get_option('enable_docs')
	doxygen = find_program('doxygen', required: false)
	dot = find_program('dot', required: false)
	if doxygen.found() and dot.found()
		run_target('doxygen', command: [doxygen, join_paths(meson.source_root(), 'Doxyfile')], build_by_default: true)
	endif
endif

foreach flag : compiler_flags
	if compiler.has_argument(flag)
		add_project_arguments(flag, language: 'c')
	endif
endforeach

# specify linker flags
linker_flags = []
if (compiler.get_id() == 'clang') or (compiler.get_id() == 'gcc')
	linker_flags += [
		'-z relro',
		'-z now',
		'-z noexecstack',
		'-z noexecheap',
		'-pie'
	]
endif

#TODO how to detect linker flags
#add_project_link_arguments(flag, language: 'c')

math = compiler.find_library('m', required: false)

subdir('lib')
subdir('test')
subdir('bindings')
