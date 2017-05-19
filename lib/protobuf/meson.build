protobuf_c = dependency('libprotobuf-c')
protoc_c = find_program('protoc-c')

protobuf_files = [
	'header',
	'packet',
	'packet_header',
	'backup',
	'conversation',
	'encrypted_backup',
	'key',
	'key_bundle',
	'prekey',
	'user'
]

protobufs = []
foreach protobuf_file : protobuf_files
	protobufs += custom_target(
		protobuf_file,
		command: [protoc_c,
			'--proto_path=' + meson.current_source_dir(),
			'--c_out=@OUTDIR@',
			join_paths(meson.current_source_dir(), protobuf_file + '.proto') ], #TODO make it work with @INPUT@ somehow
		input: protobuf_file + '.proto',
		output: ['@BASENAME@.pb-c.h', '@BASENAME@.pb-c.c']
	)
endforeach

protobuf_flag_recommendations = [
	'-Wno-cast-align'
]

protobuf_flags = []
foreach flag : protobuf_flag_recommendations
	if compiler.has_argument(flag)
		protobuf_flags += flag
	endif
endforeach

protobufs_lib = library('protobufs', protobufs, dependencies: protobuf_c, override_options: 'werror=false', c_args: protobuf_flags)
