subdir('protobuf')

lib_files = files(
	'diffie-hellman.c',
	'key-derivation.c',
	'packet.c',
	'header.c',
	'header-and-message-keystore.c',
	'ratchet.c',
	'user-store.c',
	'spiced-random.c',
	'molch.c',
	'conversation.c',
	'conversation-store.c',
	'prekey-store.c',
	'master-keys.c',
	'endianness.c',
	'return-status.c',
	'alignment.c',
	'zeroed_malloc.c',
	'buffer.c'
)

libsodium = dependency('libsodium', version: '>=1.0.12')
protobuf_include = include_directories('protobuf')
molch = library('molch', lib_files, dependencies: libsodium, link_with: protobufs_lib, include_directories: protobuf_include, install: true, version: version, soversion: soversion)
