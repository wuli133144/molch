/*
 * Molch, an implementation of the axolotl ratchet based on libsodium
 *
 * ISC License
 *
 * Copyright (C) 2015-2016 1984not Security GmbH
 * Author: Max Bruckner (FSMaxB)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <cstdio>
#include <cstdlib>
#include <sodium.h>

#include "../lib/key-derivation.h"
#include "../lib/constants.h"
#include "utils.h"
#include "common.h"

int main(void) noexcept {
	if (sodium_init() == -1) {
		return -1;
	}

	return_status status = return_status_init();

	//create key buffers
	Buffer *alice_public_ephemeral = Buffer::create(crypto_box_PUBLICKEYBYTES, crypto_box_PUBLICKEYBYTES);
	Buffer *alice_private_ephemeral = Buffer::create(crypto_box_SECRETKEYBYTES, crypto_box_SECRETKEYBYTES);
	Buffer *bob_public_ephemeral = Buffer::create(crypto_box_PUBLICKEYBYTES, crypto_box_PUBLICKEYBYTES);
	Buffer *bob_private_ephemeral = Buffer::create(crypto_box_SECRETKEYBYTES, crypto_box_SECRETKEYBYTES);
	Buffer *previous_root_key = Buffer::create(crypto_secretbox_KEYBYTES, crypto_secretbox_KEYBYTES);
	Buffer *alice_root_key = Buffer::create(crypto_secretbox_KEYBYTES, crypto_secretbox_KEYBYTES);
	Buffer *alice_chain_key = Buffer::create(crypto_secretbox_KEYBYTES, crypto_secretbox_KEYBYTES);
	Buffer *alice_header_key = Buffer::create(HEADER_KEY_SIZE, HEADER_KEY_SIZE);
	Buffer *bob_root_key = Buffer::create(crypto_secretbox_KEYBYTES, crypto_secretbox_KEYBYTES);
	Buffer *bob_chain_key = Buffer::create(crypto_secretbox_KEYBYTES, crypto_secretbox_KEYBYTES);
	Buffer *bob_header_key = Buffer::create(HEADER_KEY_SIZE, HEADER_KEY_SIZE);

	//create Alice's keypair
	buffer_create_from_string(alice_string, "Alice");
	buffer_create_from_string(ephemeral_string, "ephemeral");
	status = generate_and_print_keypair(
			alice_public_ephemeral,
			alice_private_ephemeral,
			alice_string,
			ephemeral_string);
	THROW_on_error(KEYGENERATION_FAILED, "Failed to generate and print Alice's ephemeral keypair.");

	//create Bob's keypair
	buffer_create_from_string(bob_string, "Bob");
	status = generate_and_print_keypair(
			bob_public_ephemeral,
			bob_private_ephemeral,
			bob_string,
			ephemeral_string);
	THROW_on_error(KEYGENERATION_FAILED, "Failed to generate and print Bob's ephemeral keypair.");

	//create previous root key
	if (previous_root_key->fillRandom(crypto_secretbox_KEYBYTES) != 0) {
		THROW(KEYGENERATION_FAILED, "Failed to generate previous root key.");
	}

	//print previous root key
	printf("Previous root key (%zu Bytes):\n", previous_root_key->content_length);
	print_hex(previous_root_key);
	putchar('\n');

	//derive root and chain key for Alice
	status = derive_root_next_header_and_chain_keys(
			*alice_root_key,
			*alice_header_key,
			*alice_chain_key,
			*alice_private_ephemeral,
			*alice_public_ephemeral,
			*bob_public_ephemeral,
			*previous_root_key,
			true);
	THROW_on_error(KEYDERIVATION_FAILED, "Failed to derive root, next header and chain key for Alice.");

	//print Alice's root and chain key
	printf("Alice's root key (%zu Bytes):\n", alice_root_key->content_length);
	print_hex(alice_root_key);
	printf("Alice's chain key (%zu Bytes):\n", alice_chain_key->content_length);
	print_hex(alice_chain_key);
	printf("Alice's header key (%zu Bytes):\n", alice_header_key->content_length);
	print_hex(alice_header_key);
	putchar('\n');

	//derive root and chain key for Bob
	status = derive_root_next_header_and_chain_keys(
			*bob_root_key,
			*bob_header_key,
			*bob_chain_key,
			*bob_private_ephemeral,
			*bob_public_ephemeral,
			*alice_public_ephemeral,
			*previous_root_key,
			false);
	THROW_on_error(KEYDERIVATION_FAILED, "Failed to derive root, next header and chain key for Bob.");

	//print Bob's root and chain key
	printf("Bob's root key (%zu Bytes):\n", bob_root_key->content_length);
	print_hex(bob_root_key);
	printf("Bob's chain key (%zu Bytes):\n", bob_chain_key->content_length);
	print_hex(bob_chain_key);
	printf("Bob's header key (%zu Bytes):\n", bob_header_key->content_length);
	print_hex(bob_header_key);
	putchar('\n');

	//compare Alice's and Bob's root keys
	if (alice_root_key->compare(bob_root_key) == 0) {
		printf("Alice's and Bob's root keys match.\n");
	} else {
		THROW(INCORRECT_DATA, "Alice's and Bob's root keys don't match.");
	}
	alice_root_key->clear();
	bob_root_key->clear();

	//compare Alice's and Bob's chain keys
	if (alice_chain_key->compare(bob_chain_key) == 0) {
		printf("Alice's and Bob's chain keys match.\n");
	} else {
		THROW(INCORRECT_DATA, "Alice's and Bob's chain keys don't match.");
	}

	//compare Alice's and Bob's header keys
	if (alice_header_key->compare(bob_header_key) == 0) {
		printf("Alice's and Bob's header keys match.\n");
	} else {
		THROW(INCORRECT_DATA, "Alice's and Bob's header keys don't match.");
	}

cleanup:
	buffer_destroy_from_heap_and_null_if_valid(alice_public_ephemeral);
	buffer_destroy_from_heap_and_null_if_valid(alice_private_ephemeral);
	buffer_destroy_from_heap_and_null_if_valid(bob_public_ephemeral);
	buffer_destroy_from_heap_and_null_if_valid(bob_private_ephemeral);
	buffer_destroy_from_heap_and_null_if_valid(previous_root_key);
	buffer_destroy_from_heap_and_null_if_valid(alice_root_key);
	buffer_destroy_from_heap_and_null_if_valid(alice_chain_key);
	buffer_destroy_from_heap_and_null_if_valid(alice_header_key);
	buffer_destroy_from_heap_and_null_if_valid(bob_root_key);
	buffer_destroy_from_heap_and_null_if_valid(bob_chain_key);
	buffer_destroy_from_heap_and_null_if_valid(bob_header_key);

	on_error {
		print_errors(&status);
	}
	return_status_destroy_errors(&status);

	return status.status;
}
