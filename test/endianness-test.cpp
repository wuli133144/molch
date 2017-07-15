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
#include <ctime>

#include "../lib/endianness.h"
#include "utils.h"

int main(void) {
	return_status status = return_status_init();

	Buffer *buffer64 = Buffer::create(8, 8);
	Buffer *buffer32 = Buffer::create(4, 4);

	if (endianness_is_little_endian()) {
		printf("Current byte order: Little Endian!\n");
	} else {
		printf("Current_byte_oder: Big Endian!\n");
	}

	//uint32_t -> big endian
	{
		uint32_t uint32 = 67305985ULL;
		uint32_t uint32_from_big_endian;
		status = to_big_endian(uint32, *buffer32);
		THROW_on_error(CONVERSION_ERROR, "Failed to convert uint32_t to big endian.");
		printf("uint32_t %llu to big endian:\n", (unsigned long long) uint32);
		print_hex(buffer32);

		if (buffer32->compareToRaw((const unsigned char*)"\x04\x03\x02\x01", sizeof(uint32_t)) != 0) {
			THROW(INCORRECT_DATA, "Big endian of uint32_t is incorrect.");
		}

		//uint32_t <- big endian
		status = from_big_endian(uint32_from_big_endian, *buffer32);
		THROW_on_error(CONVERSION_ERROR, "Failed to convert big endian to uint32_t.");
		if (uint32 != uint32_from_big_endian) {
			THROW(INCORRECT_DATA, "uint32_t from big endian is incorrect.");
		}
		printf("Successfully converted back!\n\n");
	}

	//int32_t -> big endian
	{
		int32_t int32 = -66052LL;
		int32_t int32_from_big_endian;
		status = to_big_endian(int32, *buffer32);
		THROW_on_error(CONVERSION_ERROR, "Failed to converst int32_t to big_endian.");
		printf("int32_t %lli to big endian:\n", (signed long long) int32);
		print_hex(buffer32);

		if (buffer32->compareToRaw((const unsigned char*)"\xFF\xFE\xFD\xFC", sizeof(int32_t)) != 0) {
			THROW(INCORRECT_DATA, "Big endian of int32_t is incorrect.");
		}

		//int32_t <- big endian
		status = from_big_endian(int32_from_big_endian, *buffer32);
		THROW_on_error(CONVERSION_ERROR, "Failed to convert big endian to int32_t.");
		if (int32 != int32_from_big_endian) {
			THROW(INCORRECT_DATA, "uint32_t from big endian is incorrect.");
		}
		printf("Successfully converted back!\n\n");
	}

	//uint64_t -> big endian
	{
		uint64_t uint64 = 578437695752307201ULL;
		uint64_t uint64_from_big_endian;
		status = to_big_endian(uint64, *buffer64);
		THROW_on_error(CONVERSION_ERROR, "Failed to convert uint64_t to big endian.");
		printf("uint64_t %llu to big endian:\n", (unsigned long long) uint64);
		print_hex(buffer64);

		if (buffer64->compareToRaw((const unsigned char*)"\x08\x07\x06\x05\x04\x03\x02\x01", sizeof(uint64_t)) != 0) {
			THROW(INCORRECT_DATA, "Big endian of uint64_t is incorrect.");
		}

		//uint64_t <- big endian
		status = from_big_endian(uint64_from_big_endian, *buffer64);
		THROW_on_error(CONVERSION_ERROR, "Failed to convert big endian to uint64_t.");
		if (uint64 != uint64_from_big_endian) {
			THROW(INCORRECT_DATA, "uint64_t from big endian is incorrect.");
		}
		printf("Successfully converted back!\n\n");
	}

	//int64_t -> big endian
	{
		int64_t int64 = -283686952306184LL;
		int64_t int64_from_big_endian;
		status = to_big_endian(int64, *buffer64);
		THROW_on_error(CONVERSION_ERROR, "Failed to converst int64_t to big endian.");
		printf("int64_t %lli to big endian:\n", (signed long long) int64);
		print_hex(buffer64);

		if (buffer64->compareToRaw((const unsigned char*)"\xFF\xFE\xFD\xFC\xFB\xFA\xF9\xF8", sizeof(int64_t)) != 0) {
			THROW(INCORRECT_DATA, "Big endian of int64_t is incorrect.");
		}

		//int64_t <- big endian
		status = from_big_endian(int64_from_big_endian, *buffer64);
		THROW_on_error(CONVERSION_ERROR, "Failed to convert big endian to int64_t.");
		if (int64 != int64_from_big_endian) {
			THROW(INCORRECT_DATA, "unit64_t from big endian is incorrect.");
		}
		printf("Successfully converted back!\n\n");
	}

cleanup:
	buffer_destroy_from_heap_and_null_if_valid(buffer64);
	buffer_destroy_from_heap_and_null_if_valid(buffer32);

	on_error {
		print_errors(&status);
	}
	return_status_destroy_errors(&status);

	return status.status;
}
