test_files = [
	'diffie-hellman-test',
	'triple-diffie-hellman-test',
	'key-derivation-test',
	'chain-key-derivation-test',
	'message-key-derivation-test',
	'root-next-header-and-chain-key-derivation-test',
	'initial-root-chain-and-header-key-derivation-test',
	'packet-get-metadata-test',
	'packet-decrypt-header-test',
	'packet-decrypt-message-test',
	'packet-decrypt-test',
	'header-test',
	'header-and-message-keystore-test',
	'ratchet-test',
	'ratchet-test-simple',
	'user-store-test',
	'spiced-random-test',
	'molch-test',
	'conversation-test',
	'conversation-packet-test',
	'conversation-store-test',
	'prekey-store-test',
	'master-keys-test',
	'endianness-test',
	'return-status-test',
	'molch-init-test',
	'alignment-test',
	'zeroed_malloc-test',
	'buffer-test'
]

test_utils = library('test-utils', ['utils.c'], include_directories: protobuf_include, link_with: [molch], dependencies: libsodium)
test_common = library('test_common', ['common.c'], include_directories: protobuf_include, link_with: [molch, test_utils], dependencies: libsodium)
packet_test_lib = library('packet-test-lib', 'packet-test-lib.c', link_with: [test_utils, test_common])


foreach test : test_files
	test_executable = executable(test, test + '.c', link_with: packet_test_lib, include_directories: protobuf_include, dependencies: [libsodium, protobuf_c])
	test(test, test_executable, workdir: meson.current_source_dir())
endforeach
