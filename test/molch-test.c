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
#include <stdio.h>
#include <stdlib.h>
#include <sodium.h>

#include "utils.h"
#include "../lib/molch.h"
#include "../lib/user-store.h" //for PREKEY_AMOUNT
#include "tracing.h"

int main(void) {
	if (sodium_init() == -1) {
		return -1;
	}

	//mustn't crash here!
	molch_destroy_all_users();

	//check user count
	if (molch_user_count() != 0) {
		fprintf(stderr, "ERROR: Wrong user count.\n");
		return EXIT_FAILURE;
	}

	int status;
	//create conversation buffers
	buffer_t *alice_conversation = buffer_create_on_heap(CONVERSATION_ID_SIZE, CONVERSATION_ID_SIZE);
	buffer_t *bob_conversation = buffer_create_on_heap(CONVERSATION_ID_SIZE, CONVERSATION_ID_SIZE);

	//alice key buffers
	buffer_t *alice_public_identity = buffer_create_on_heap(crypto_box_PUBLICKEYBYTES, crypto_box_PUBLICKEYBYTES);
	unsigned char *alice_public_prekeys = NULL;
	size_t alice_public_prekeys_length = 0;

	//bobs key buffers
	buffer_t *bob_public_identity = buffer_create_on_heap(crypto_box_PUBLICKEYBYTES, crypto_box_PUBLICKEYBYTES);
	unsigned char *bob_public_prekeys = NULL;
	size_t bob_public_prekeys_length = 0;

	//packet pointers
	unsigned char * alice_send_packet = NULL;
	unsigned char * bob_send_packet = NULL;

	//create a new user
	buffer_create_from_string(alice_head_on_keyboard, "mn ujkhuzn7b7bzh6ujg7j8hn");
	status = molch_create_user(
			alice_public_identity->content,
			&alice_public_prekeys,
			&alice_public_prekeys_length,
			alice_head_on_keyboard->content,
			alice_head_on_keyboard->content_length);
	if (status != 0) {
		fprintf(stderr, "ERROR: Failed to create Alice! (%i)\n", status);
		goto cleanup;
	}
	printf("Alice public identity (%zu Bytes):\n", alice_public_identity->content_length);
	print_hex(alice_public_identity);
	putchar('\n');

	//check user count
	if (molch_user_count() != 1) {
		fprintf(stderr, "ERROR: Wrong user count.\n");
		status = EXIT_FAILURE;
		goto cleanup;
	}

	//create another user
	buffer_create_from_string(bob_head_on_keyboard, "jnu8h77z6ht56ftgnujh");
	status = molch_create_user(
			bob_public_identity->content,
			&bob_public_prekeys,
			&bob_public_prekeys_length,
			bob_head_on_keyboard->content,
			bob_head_on_keyboard->content_length);
	if (status != 0) {
		fprintf(stderr, "ERROR: Failed to create Bob! (%i)\n", status);
		goto cleanup;
	}
	printf("Bob public identity (%zu Bytes):\n", bob_public_identity->content_length);
	print_hex(bob_public_identity);
	putchar('\n');

	//check user count
	if (molch_user_count() != 2) {
		fprintf(stderr, "ERROR: Wrong user count.\n");
		status = EXIT_FAILURE;
		goto cleanup;
	}

	//check user list
	size_t user_count;
	unsigned char *user_list = molch_user_list(&user_count);
	if ((user_count != 2)
			|| (sodium_memcmp(alice_public_identity->content, user_list, alice_public_identity->content_length) != 0)
			|| (sodium_memcmp(bob_public_identity->content, user_list + crypto_box_PUBLICKEYBYTES, alice_public_identity->content_length) != 0)) {
		fprintf(stderr, "ERROR: Wrong user list.\n");
		free(user_list);
		status = EXIT_FAILURE;
		goto cleanup;
	}
	free(user_list);

	//create a new send conversation (alice sends to bob)
	buffer_create_from_string(alice_send_message, "Hi Bob. Alice here!");
	size_t alice_send_packet_length;
	status = molch_create_send_conversation(
			alice_conversation->content,
			&alice_send_packet,
			&alice_send_packet_length,
			alice_send_message->content,
			alice_send_message->content_length,
			bob_public_prekeys,
			bob_public_prekeys_length,
			alice_public_identity->content,
			bob_public_identity->content);
	if (status != 0) {
		fprintf(stderr, "ERROR: Failed to start send conversation. (%i)\n", status);
		goto cleanup;
	}

	//check conversation export
	size_t number_of_conversations;
	unsigned char *conversation_list = molch_list_conversations(alice_public_identity->content, &number_of_conversations);
	if (conversation_list == NULL) {
		fprintf(stderr, "ERROR: Failed to list conversations.\n");
		status = EXIT_FAILURE;
		goto cleanup;
	}
	if ((number_of_conversations != 1) || (buffer_compare_to_raw(alice_conversation, conversation_list, alice_conversation->content_length) != 0)) {
		fprintf(stderr, "ERROR: Failed to list conversations.\n");
		free(conversation_list);
		status = EXIT_FAILURE;
		goto cleanup;
	}
	free(conversation_list);

	//check the message type
	if (molch_get_message_type(alice_send_packet, alice_send_packet_length) != PREKEY_MESSAGE) {
		fprintf(stderr, "ERROR: Wrong message type.\n");
		status = EXIT_FAILURE;
		goto cleanup;
	}

	if (bob_public_prekeys != NULL) {
		free(bob_public_prekeys);
		bob_public_prekeys = NULL;
	}
	//create a new receive conversation (bob receives from alice)
	unsigned char *bob_receive_message;
	size_t bob_receive_message_length;
	status = molch_create_receive_conversation(
			bob_conversation->content,
			&bob_receive_message,
			&bob_receive_message_length,
			alice_send_packet,
			alice_send_packet_length,
			&bob_public_prekeys,
			&bob_public_prekeys_length,
			alice_public_identity->content,
			bob_public_identity->content);
	if (status != 0) {
		fprintf(stderr, "ERROR: Failed to start receive conversation. (%i)\n", status);
		goto cleanup;
	}

	//compare sent and received messages
	printf("sent (Alice): %s\n", alice_send_message->content);
	printf("received (Bob): %s\n", bob_receive_message);
	if ((alice_send_message->content_length != bob_receive_message_length)
			|| (sodium_memcmp(alice_send_message->content, bob_receive_message, bob_receive_message_length) != 0)) {
		fprintf(stderr, "ERROR: Incorrect message received.\n");
		free(bob_receive_message);
		status = EXIT_FAILURE;
		goto cleanup;
	}
	free(bob_receive_message);

	//bob replies
	buffer_create_from_string(bob_send_message, "Welcome Alice!");
	size_t bob_send_packet_length;
	status = molch_encrypt_message(
			&bob_send_packet,
			&bob_send_packet_length,
			bob_send_message->content,
			bob_send_message->content_length,
			bob_conversation->content);
	if (status != 0) {
		fprintf(stderr, "ERROR: Couldn't send bobs message.\n");
		goto cleanup;
	}

	//check the message type
	if (molch_get_message_type(bob_send_packet, bob_send_packet_length) != NORMAL_MESSAGE) {
		fprintf(stderr, "ERROR: Wrong message type.\n");
		status = EXIT_FAILURE;
		goto cleanup;
	}

	//alice receives reply
	unsigned char *alice_receive_message;
	size_t alice_receive_message_length;
	status = molch_decrypt_message(
			&alice_receive_message,
			&alice_receive_message_length,
			bob_send_packet,
			bob_send_packet_length,
			alice_conversation->content);
	if (status != 0) {
		fprintf(stderr, "ERROR: Incorrect message received.\n");
		free(alice_receive_message);
		goto cleanup;
	}

	//compare sent and received messages
	printf("sent (Bob): %s\n", bob_send_message->content);
	printf("received (Alice): %s\n", alice_receive_message);
	if ((bob_send_message->content_length != alice_receive_message_length)
			|| (sodium_memcmp(bob_send_message->content, alice_receive_message, alice_receive_message_length) != 0)) {
		fprintf(stderr, "ERROR: Incorrect message received.\n");
		free(alice_receive_message);
		status = EXIT_FAILURE;
		goto cleanup;
	}
	free(alice_receive_message);

	//test JSON export
	printf("Test JSON export:\n");
	size_t json_length;
	unsigned char *json = molch_json_export(&json_length);
	if (json == NULL) {
		fprintf(stderr, "ERROR: Failed to export to JSON.\n");
		status = EXIT_FAILURE;
		goto cleanup;
	}
	printf("%.*s\n", (int)json_length, json);

	//test JSON import
	printf("Test JSON import:\n");
	status = molch_json_import(json, json_length);
	if (status != 0) {
		fprintf(stderr, "ERROR: Failed to import JSON. (%i)\n", status);
		sodium_free(json);
		goto cleanup;
	}
	//now export again
	size_t imported_json_length;
	unsigned char *imported_json = molch_json_export(&imported_json_length);
	if (imported_json == NULL) {
		fprintf(stderr, "ERROR: Failed to export imported JSON.\n");
		sodium_free(json);
		status = EXIT_FAILURE;
		goto cleanup;
	}
	//compare
	if ((json_length != imported_json_length) || (sodium_memcmp(json, imported_json, json_length) != 0)) {
		fprintf(stderr, "ERROR: Imported JSON is incorrect.\n");
		sodium_free(json);
		sodium_free(imported_json);
		status = EXIT_FAILURE;
		goto cleanup;
	}
	sodium_free(json);
	sodium_free(imported_json);

	//test conversation JSON export
	json = molch_conversation_json_export(alice_conversation->content, &json_length);
	if (json == NULL) {
		fprintf(stderr, "ERROR: Failed to export Alice' convesation as JSON!\n");
		status = EXIT_FAILURE;
		goto cleanup;
	}
	printf("Alice' conversation exported to JSON:\n");
	printf("%.*s\n", (int)json_length, (char*)json);

	sodium_free(json);

	//destroy the conversations
	molch_end_conversation(alice_conversation->content);
	molch_end_conversation(bob_conversation->content);

	//destroy the users again
	molch_destroy_all_users();

	//check user count
	if (molch_user_count() != 0) {
		fprintf(stderr, "ERROR: Wrong user count.\n");
		status = EXIT_FAILURE;
		goto cleanup;
	}

	//TODO check detection of invalid prekey list signatures and old timestamps + more scenarios


cleanup:
	if (alice_public_prekeys != NULL) {
		free(alice_public_prekeys);
	}
	if (bob_public_prekeys != NULL) {
		free(bob_public_prekeys);
	}
	if (alice_send_packet != NULL) {
		free(alice_send_packet);
	}
	if (bob_send_packet != NULL) {
		free(bob_send_packet);
	}
	molch_destroy_all_users();
	buffer_destroy_from_heap(alice_conversation);
	buffer_destroy_from_heap(bob_conversation);
	buffer_destroy_from_heap(alice_public_identity);
	buffer_destroy_from_heap(bob_public_identity);

	return status;
}
