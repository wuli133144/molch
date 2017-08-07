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

extern "C" {
	#include <user.pb-c.h>
}

#include <memory>

#include "constants.h"
#include "buffer.h"
#include "return-status.h"
#include "zeroed_malloc.h"
#include "sodium-wrappers.h"
#include "protobuf-deleters.h"

#ifndef LIB_MASTER_KEYS
#define LIB_MASTER_KEYS

class PrivateMasterKeyStorage {
	friend class MasterKeys;
	unsigned char signing_key[PRIVATE_MASTER_KEY_SIZE];
	unsigned char identity_key[PRIVATE_KEY_SIZE];
};

class MasterKeys {
private:
	std::unique_ptr<PrivateMasterKeyStorage,SodiumDeleter<PrivateMasterKeyStorage>> private_keys;

    /* Internally does the intialization of the buffers creation of the keys */
    void init();
    void generate(const Buffer* low_entropy_seed);

	enum {LOCKED, READONLY, READWRITE} lock_state;
	void unlock_readwrite();

public:
	//Ed25519 key for signing
	Buffer public_signing_key;
	Buffer private_signing_key;
	//X25519 key for deriving axolotl root keys
	Buffer public_identity_key;
	Buffer private_identity_key;

	/*
	 * Create a new set of master keys.
	 *
	 * Optionally wit a seed. It can be of any length and doesn't
	 * require to have high entropy. It will be used as entropy source
	 * in addition to the OSs CPRNG.
	 *
	 * WARNING: Don't use Entropy from the OSs CPRNG as seed!
	 */
	MasterKeys();
	MasterKeys(const Buffer& low_entropy_seed);

	/*
	 * import from Protobuf-C
	 */
	MasterKeys(
		const Key& public_signing_key,
		const Key& private_signing_key,
		const Key& public_identity_key,
		const Key& private_identity_key);

	/*
	 * Get the public signing key.
	 */
	void getSigningKey(Buffer& public_signing_key);

	/*
	 * Get the public identity key.
	 */
	void getIdentityKey(Buffer& public_identity_key);

	/*
	 * Sign a piece of data. Returns the data and signature in one output buffer.
	 */
	void sign(const Buffer& data, Buffer& signed_data); //output, length of data + SIGNATURE_SIZE

	/*! Export a set of master keys into a user Protobuf-C struct
	 * \param public_signing_key Public pasrt of the signing keypair.
	 * \param private_signing_key Private part of the signing keypair.
	 * \param public_identity_key Public part of the identity keypair.
	 * \param private_identity_key Private part of the idenity keypair.
	 */
	void exportProtobuf(
            std::unique_ptr<Key,KeyDeleter>& public_signing_key,
            std::unique_ptr<Key,KeyDeleter>& private_signing_key,
            std::unique_ptr<Key,KeyDeleter>& public_identity_key,
            std::unique_ptr<Key,KeyDeleter>& private_identity_key);

	/* Manage the memory for the private keys */
	void lock();
	void unlock();
};

#endif
