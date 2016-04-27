/* Molch, an implementation of the axolotl ratchet based on libsodium
 *  Copyright (C) 2015  Max Bruckner (FSMaxB)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "constants.h"
#include "return-status.h"
#include "../buffer/buffer.h"
#include "../mcJSON/mcJSON.h"

#ifndef LIB_MASTER_KEYS
#define LIB_MASTER_KEYS

typedef struct master_keys {
	//Ed25519 key for signing
	buffer_t public_signing_key[1];
	unsigned char public_signing_key_storage[PUBLIC_MASTER_KEY_SIZE];
	buffer_t private_signing_key[1];
	unsigned char private_signing_key_storage[PRIVATE_MASTER_KEY_SIZE];
	//X25519 key for deriving axolotl root keys
	buffer_t public_identity_key[1];
	unsigned char public_identity_key_storage[PUBLIC_KEY_SIZE];
	buffer_t private_identity_key[1];
	unsigned char private_identity_key_storage[PRIVATE_KEY_SIZE];
} master_keys;

/*
 * Create a new set of master keys.
 *
 * Seed is optional, can be NULL. It can be of any length and doesn't
 * require to have high entropy. It will be used as entropy source
 * in addition to the OSs CPRNG.
 *
 * WARNING: Don't use Entropy from the OSs CPRNG as seed!
 */
return_status master_keys_create(
		master_keys ** const keys, //output
		const buffer_t * const seed,
		buffer_t * const public_signing_key, //output, optional, can be NULL
		buffer_t * const public_identity_key //output, optional, can be NULL
		) __attribute__((warn_unused_result));

/*
 * Get the public signing key.
 */
return_status master_keys_get_signing_key(
		master_keys * const keys,
		buffer_t * const public_signing_key) __attribute__((warn_unused_result));

/*
 * Get the public identity key.
 */
return_status master_keys_get_identity_key(
		master_keys * const keys,
		buffer_t * const public_identity_key) __attribute__((warn_unused_result));

/*
 * Sign a piece of data. Returns the data and signature in one output buffer.
 */
return_status master_keys_sign(
		master_keys * const keys,
		const buffer_t * const data,
		buffer_t * const signed_data //output, length of data + SIGNATURE_SIZE
		) __attribute__((warn_unused_result));

/*
 * Serialise the master keys into JSON. It get's a mempool_t buffer and stores mcJSON
 * Objects into it starting at pool->position.
 */
mcJSON *master_keys_json_export(master_keys * const keys, mempool_t * const pool) __attribute__((warn_unused_result));

/*
 * Deserialize a set of master keys (import from JSON).
 */
master_keys *master_keys_json_import(const mcJSON * const json) __attribute__((warn_unused_result));

#endif
