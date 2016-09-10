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

void read_file(buffer_t ** const data, const char * const filename) {
	if (data == NULL) {
		return;
	}

	*data = NULL;

	FILE *file = NULL;

	file = fopen(filename, "r");

	//get the filesize
	fseek(file, 0, SEEK_END);
	long filesize = ftell(file);
	fseek(file, 0, SEEK_SET);

	*data = buffer_create_on_heap(filesize, filesize);
	(*data)->content_length = fread((*data)->content, 1, filesize, file);

	fclose(file);
}
