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

#include <sodium.h>
#include <assert.h>

#include "constants.h"
#include "spiced-random.h"
#include "return-status.h"

/*
 * Generate a random number by combining the OSs random number
 * generator with an external source of randomness (like some kind of
 * user input).
 *
 * WARNING: Don't feed this with random numbers from the OSs random
 * source because it might annihilate the randomness.
 */
return_status spiced_random(
		buffer_t * const random_output,
		const buffer_t * const random_spice,
		const size_t output_length) {
	return_status status = return_status_init();

	//buffer to put the random data derived from the random spice into
	buffer_t *spice = NULL;
	//buffer that contains the random data from the OS
	buffer_t *os_random = NULL;
	//allocate them
	spice = buffer_create_with_custom_allocator(output_length, output_length, sodium_malloc, sodium_free);
	throw_on_failed_alloc(spice);
	os_random = buffer_create_with_custom_allocator(output_length, output_length, sodium_malloc, sodium_free);
	throw_on_failed_alloc(os_random);

	//check buffer length
	if (random_output->buffer_length < output_length) {
		throw(INCORRECT_BUFFER_SIZE, "Output buffers is too short.");
	}

	if (buffer_fill_random(os_random, output_length) != 0) {
		throw(GENERIC_ERROR, "Failed to fill buffer with random data.");
	}

	buffer_create_from_string(salt, " molch: an axolotl ratchet lib ");
	assert(salt->content_length == crypto_pwhash_scryptsalsa208sha256_SALTBYTES);

	//derive random data from the random spice
	int status_int = 0;
	status_int = crypto_pwhash_scryptsalsa208sha256(
			spice->content,
			spice->content_length,
			(const char*)random_spice->content,
			random_spice->content_length,
			salt->content,
			crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE,
			crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE);
	if (status_int != 0) {
		throw(GENERIC_ERROR, "Failed to derive random data from spice.");
	}

	//now combine the spice with the OS provided random data.
	if (buffer_xor(os_random, spice) != 0) {
		throw(GENERIC_ERROR, "Failed to xor os random data and random data derived from spice.");
	}

	//copy the random data to the output
	if (buffer_clone(random_output, os_random) != 0) {
		throw(BUFFER_ERROR, "Failed to copy random data.");
	}

cleanup:
	on_error {
		if (random_output != NULL) {
			buffer_clear(random_output);
			random_output->content_length = 0;
		}
	}
	buffer_destroy_with_custom_deallocator_and_null_if_valid(spice, sodium_free);
	buffer_destroy_with_custom_deallocator_and_null_if_valid(os_random, sodium_free);

	return status;
}
