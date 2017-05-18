swig = find_program('swig', required: false)

swig_flag_recommendations = [
	'-Wno-error',
	'-Wno-conversion'
]
swig_flags = []
foreach flag : swig_flag_recommendations
	if compiler.has_argument(flag)
		swig_flags += flag
	endif
endforeach

swig_molch_wrapper = custom_target(
	'swig_molch_wrapper',
	command: [swig,
		'-lua',
		'-I' + meson.source_root(),
		'-o',
		'@OUTPUT@',
		'@INPUT@'],
	input: 'molch.i',
	output: '@BASENAME@_wrap.c')

liblua = dependency('lua', version: '>=5.3')
root_include = include_directories('../')
molch_interface = library('molch-interface', swig_molch_wrapper, c_args: swig_flags, include_directories: root_include, link_with: molch, dependencies: [liblua, libsodium])
