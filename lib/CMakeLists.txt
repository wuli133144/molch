cmake_minimum_required (VERSION 2.6)

subdirs(protobuf)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/protobuf")

add_library(molch
	diffie-hellman
	key-derivation
	packet
	header
	header-and-message-keystore
	ratchet user-store
	spiced-random
	molch
	conversation
	conversation-store
	prekey-store
	master-keys
	endianness
	return-status
	alignment
	zeroed_malloc
)
target_link_libraries(molch ${libs} molch-buffer protocol-buffers)
