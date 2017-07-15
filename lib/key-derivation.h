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

#include "buffer.h"
#include "common.h"

#ifndef LIB_KEY_DERIVATION_H
#define LIB_KEY_DERIVATION_H

/*
 * Derive a key of length between crypto_generichash_blake2b_BYTES_MIN (16 Bytes)
 * and crypto_generichash_blake2b_BYTES_MAX (64 Bytes).
 *
 * The input key needs to be between crypto_generichash_blake2b_KEYBYTES_MIN (16 Bytes)
 * and crypto_generichash_blake2b_KEYBYTES_MAX (64 Bytes).
 */
return_status derive_key(
		Buffer& derived_key,
		size_t derived_size,
		Buffer& input_key,
		uint32_t subkey_counter) __attribute__((warn_unused_result)); //number of the current subkey, used to derive multiple keys from the same input key

/*
 * Derive the next chain key in a message chain.
 *
 * The chain keys have to be crypto_auth_BYTES long.
 *
 * CK_new = HMAC-Hash(CK_prev, 0x01)
 * (previous chain key as key, 0x01 as message)
 */
return_status derive_chain_key(
		Buffer& new_chain_key,
		Buffer& previous_chain_key) __attribute__((warn_unused_result));

/*
 * Derive a message key from a chain key.
 *
 * The chain and message keys have to be crypto_auth_BYTES long.
 *
 * MK = HMAC-Hash(CK, 0x00)
 * (chain_key as key, 0x00 as message)
 */
return_status derive_message_key(
		Buffer& message_key,
		Buffer& chain_key) __attribute__((warn_unused_result));

/*
 * Derive a root, next header and initial chain key for a new ratchet.
 *
 * RK, NHKs, CKs = KDF(HMAC-HASH(RK, DH(DHRr, DHRs)))
 * and
 * RK, NHKp, CKp = KDF(HMAC-HASH(RK, DH(DHRp, DHRs)))
 */
return_status derive_root_next_header_and_chain_keys(
		Buffer& root_key, //ROOT_KEY_SIZE
		Buffer& next_header_key, //HEADER_KEY_SIZE
		Buffer& chain_key, //CHAIN_KEY_SIZE
		Buffer& our_private_ephemeral,
		Buffer& our_public_ephemeral,
		Buffer& their_public_ephemeral,
		Buffer& previous_root_key,
		bool am_i_alice) __attribute__((warn_unused_result));

/*
 * Derive initial root, chain and header keys.
 *
 * RK, CKs/r, HKs/r = KDF(HASH(DH(A,B0) || DH(A0,B) || DH(A0,B0)))
 */
return_status derive_initial_root_chain_and_header_keys(
		Buffer& root_key, //ROOT_KEY_SIZE
		Buffer& send_chain_key, //CHAIN_KEY_SIZE
		Buffer& receive_chain_key, //CHAIN_KEY_SIZE
		Buffer& send_header_key, //HEADER_KEY_SIZE
		Buffer& receive_header_key, //HEADER_KEY_SIZE
		Buffer& next_send_header_key, //HEADER_KEY_SIZE
		Buffer& next_receive_header_key, //HEADER_KEY_SIZE
		Buffer& our_private_identity,
		Buffer& our_public_identity,
		Buffer& their_public_identity,
		Buffer& our_private_ephemeral,
		Buffer& our_public_ephemeral,
		Buffer& their_public_ephemeral,
		bool am_i_alice) __attribute__((warn_unused_result));

#endif
