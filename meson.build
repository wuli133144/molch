project('molch', 'c', default_options: ['c_std=c99'])
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
		'-Werror',
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
