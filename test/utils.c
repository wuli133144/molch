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

#include <stdio.h>
#include <stdlib.h>
#include <sodium.h>

#include "utils.h"
#include "../lib/common.h"

void print_hex(const buffer_t * const data) {
	static const int WIDTH = 30;
	//buffer for hex string
	buffer_t *hex = buffer_create_on_heap(2 * data->content_length + 1, 2 * data->content_length + 1);

	if (buffer_clone_as_hex(hex, data) != 0) {
		fprintf(stderr, "ERROR: Failed printing hex.\n");
		buffer_destroy_from_heap_and_null_if_valid(hex);
		return;
	}

	for (size_t i = 0; i < 2 * data->content_length; i++) {
		if ((WIDTH != 0) && (i % WIDTH == 0) && (i != 0)) {
			putchar('\n');
		} else if ((i % 2 == 0) && (i != 0)) {
			putchar(' ');
		}
		putchar(hex->content[i]);
	}

	putchar('\n');

	//cleanup
	buffer_destroy_from_heap_and_null_if_valid(hex);
}

void print_to_file(const buffer_t * const data, const char * const filename) {
	FILE *file = fopen(filename, "w");
	if (file == NULL) {
		return;
	}

	fwrite(data->content, 1, data->content_length, file);

	fclose(file);
}

void print_errors(return_status * const status) {
	if (status == NULL) {
		return;
	}

	fprintf(stderr, "ERROR STACK:\n");
	error_message *error = status->error;
	for (size_t i = 1; error != NULL; i++, error = error->next) {
		fprintf(stderr, "%zu: %s\n", i, error->message);
	}
}


return_status read_file(buffer_t ** const data, const char * const filename) {
	return_status status = return_status_init();

	FILE *file = NULL;

	//check input
	if ((data == NULL) || (filename == NULL)) {
		throw(INVALID_INPUT, "Invalid input to read_file.");
	}

	*data = NULL;

	file = fopen(filename, "r");
	throw_on_failed_alloc(file);

	//get the filesize
	fseek(file, 0, SEEK_END);
	long filesize = ftell(file);
	fseek(file, 0, SEEK_SET);

	*data = buffer_create_on_heap(filesize, filesize);
	throw_on_failed_alloc(*data);
	(*data)->content_length = fread((*data)->content, 1, filesize, file);
	if ((*data)->content_length != (size_t)filesize) {
		throw(INCORRECT_DATA, "Read less data from file than filesize.");
	}

cleanup:
	on_error {
		if (data != NULL) {
			buffer_destroy_from_heap_and_null_if_valid(*data);
		}
	}

	if (file != NULL) {
		fclose(file);
		file = NULL;
	}

	return status;
}
