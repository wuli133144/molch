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

//! \file Common macros.

#ifndef LIB_MACROS_H
#define LIB_MACROS_H

// execute code if a pointer is not nullptr
#define if_valid(pointer) if ((pointer) != nullptr)
// macros that free memory and delete the pointer afterwards
#define free_and_null_if_valid(pointer)\
	if_valid(pointer) {\
		free(pointer);\
		pointer = nullptr;\
	}
#define sodium_free_and_null_if_valid(pointer)\
	if_valid(pointer) {\
		sodium_free(pointer);\
		pointer = nullptr;\
	}
#define zeroed_free_and_null_if_valid(pointer)\
	if_valid(pointer) {\
		zeroed_free(pointer);\
		pointer = nullptr;\
	}
#define buffer_destroy_from_heap_and_null_if_valid(buffer)\
	if_valid(buffer) {\
		(buffer)->destroy_from_heap();\
		buffer = nullptr;\
	}
#define buffer_destroy_with_custom_deallocator_and_null_if_valid(buffer, deallocator)\
	if_valid(buffer) {\
		(buffer)->destroy_with_custom_deallocator(deallocator);\
		buffer = nullptr;\
	}

#endif
