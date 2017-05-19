if get_option('enable_bindings')
	if get_option('enable_valgrind')
		error('FIXME: Currently enabling both valgrind and the Lua bindings is not supported')
	endif
	swig = find_program('swig', required: false)

	swig_flag_recommendations = [
		'-Wno-conversion',
		'-Wno-unused-macros',
		'-Wno-cast-qual',
		'-Wno-missing-prototypes'
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

	liblua = dependency('lua', version: '>=5.2')
	root_include = include_directories('../')
	molch_interface = library('molch-interface', swig_molch_wrapper, c_args: swig_flags, include_directories: root_include, link_with: molch, dependencies: [liblua, libsodium], name_prefix: '', override_options: ['werror=false'])

	#install molch.lua
	empty_data = configuration_data()
	configure_file(input: 'molch.lua', output: 'molch.lua', configuration: empty_data)

	#run lua tests
	# ['scenario-name', should_fail]
	scenarios = [
		['normal-conversation', false],
		['reordering', false],
		['invalid_message', false],
		['double-import', false],
		['restart', false],
		['fail', true],
	]

	lua = find_program('lua5.3', 'lua51', 'lua5.2', 'lua52', 'lua')
	foreach scenario : scenarios
		scenario_args = files(join_paths('scenarios', 'scenarios.lua'), join_paths('scenarios', scenario.get(0)))
		test('scenario: ' + scenario.get(0), lua, args: scenario_args, workdir: meson.current_build_dir(), should_fail: scenario.get(1))
	endforeach
endif
