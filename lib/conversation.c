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
#include "conversation.h"
#include "molch.h"
#include "packet.h"
#include "header.h"

/*
 * Create a new conversation struct and initialise the buffer pointer.
 */
void init_struct(conversation_t *conversation) {
	buffer_init_with_pointer(conversation->id, conversation->id_storage, CONVERSATION_ID_SIZE, CONVERSATION_ID_SIZE);
	conversation->ratchet = NULL;
}

/*
 * Create a new conversation
 */
int conversation_init(
		conversation_t * const conversation,
		const buffer_t * const our_private_identity,
		const buffer_t * const our_public_identity,
		const buffer_t * const their_public_identity,
		const buffer_t * const our_private_ephemeral,
		const buffer_t * const our_public_ephemeral,
		const buffer_t * const their_public_ephemeral) {
	init_struct(conversation);

	//create random id
	if (buffer_fill_random(conversation->id, CONVERSATION_ID_SIZE) != 0) {
		sodium_memzero(conversation, sizeof(conversation_t));
		return -1;
	}

	ratchet_state *ratchet = ratchet_create(
			our_private_identity,
			our_public_identity,
			their_public_identity,
			our_private_ephemeral,
			our_public_ephemeral,
			their_public_ephemeral);
	if (ratchet == NULL) {
		sodium_memzero(conversation, sizeof(conversation_t));
		return -2;
	}

	conversation->ratchet = ratchet;

	return 0;
}

/*
 * Destroy a conversation.
 */
void conversation_deinit(conversation_t * const conversation) {
	if (conversation->ratchet != NULL) {
		ratchet_destroy(conversation->ratchet);
	}
	sodium_memzero(conversation, sizeof(conversation_t));
}

/*
 * Serialise a conversation into JSON. It get#s a mempool_t buffer and stores a tree of
 * mcJSON objects into the buffer starting at pool->position.
 *
 * Returns NULL in case of failure.
 */
mcJSON *conversation_json_export(const conversation_t * const conversation, mempool_t * const pool) {
	if ((conversation == NULL) || (pool == NULL)) {
		return NULL;
	}

	mcJSON *json = mcJSON_CreateObject(pool);
	if (json == NULL) {
		return NULL;
	}

	mcJSON *id = mcJSON_CreateHexString(conversation->id, pool);
	if (id == NULL) {
		return NULL;
	}
	mcJSON *ratchet = ratchet_json_export(conversation->ratchet, pool);
	if (ratchet == NULL) {
		return NULL;
	}

	buffer_create_from_string(id_string, "id");
	mcJSON_AddItemToObject(json, id_string, id, pool);
	buffer_create_from_string(ratchet_string, "ratchet");
	mcJSON_AddItemToObject(json, ratchet_string, ratchet, pool);

	return json;
}

/*
 * Deserialize a conversation (import from JSON)
 */
int conversation_json_import(
		const mcJSON * const json,
		conversation_t * const conversation) {
	if ((json == NULL) || (json->type != mcJSON_Object)) {
		return -2;
	}

	init_struct(conversation);

	//import the json
	buffer_create_from_string(id_string, "id");
	mcJSON *id = mcJSON_GetObjectItem(json, id_string);
	buffer_create_from_string(ratchet_string, "ratchet");
	mcJSON *ratchet = mcJSON_GetObjectItem(json, ratchet_string);
	if ((id == NULL) || (id->type != mcJSON_String) || (id->valuestring->content_length != (2 * CONVERSATION_ID_SIZE + 1))
			|| (ratchet == NULL) || (ratchet->type != mcJSON_Object)) {
		goto fail;
	}

	//copy the id
	if (buffer_clone_from_hex(conversation->id, id->valuestring) != 0) {
		goto fail;
	}

	//import the ratchet state
	conversation->ratchet = ratchet_json_import(ratchet);
	if (conversation->ratchet == NULL) {
		goto fail;
	}

	return 0;
fail:
	conversation_deinit(conversation);
	return -1;
}

/*
 * Start a new conversation where we are the sender.
 */
int conversation_start_send_conversation(
		conversation_t *const conversation, //conversation to initialize
		const buffer_t *const message, //message we want to send to the receiver
		buffer_t ** packet, //output, free after use!
		const buffer_t * const sender_public_identity, //who is sending this message?
		const buffer_t * const sender_private_identity,
		const buffer_t * const sender_public_ephemeral,
		const buffer_t * const sender_private_ephemeral,
		const buffer_t * const receiver_public_identity,
		const buffer_t * const receiver_public_ephemeral) {
	//check many error conditions
	if ((conversation == NULL)
			|| (message == NULL)
			|| (packet == NULL)
			|| (receiver_public_identity == NULL) || (receiver_public_identity->content_length != PUBLIC_KEY_SIZE)
			|| (sender_public_identity == NULL) || (sender_public_identity->content_length != PUBLIC_KEY_SIZE)
			|| (sender_private_identity == NULL) || (sender_private_identity->content_length != PRIVATE_KEY_SIZE)
			|| (receiver_public_ephemeral == NULL) || (receiver_public_ephemeral->content_length != PUBLIC_KEY_SIZE)) {
		return -1;
	}

	int status = 0;

	//create buffers
	buffer_t *send_header_key = buffer_create_on_heap(HEADER_KEY_SIZE, HEADER_KEY_SIZE);
	buffer_t *send_message_key = buffer_create_on_heap(MESSAGE_KEY_SIZE, MESSAGE_KEY_SIZE);
	buffer_t *send_ephemeral = buffer_create_on_heap(PUBLIC_KEY_SIZE, 0);
	buffer_t *header = buffer_create_on_heap(PUBLIC_KEY_SIZE + 8, PUBLIC_KEY_SIZE + 8);

	//initialize the conversation
	status = conversation_init(
			conversation,
			sender_private_identity,
			sender_public_identity,
			receiver_public_identity,
			sender_private_ephemeral,
			sender_public_ephemeral,
			receiver_public_ephemeral);
	if (status != 0) {
		goto cleanup;
	}

	//get keys for encrypting the packet
	uint32_t send_message_number;
	uint32_t previous_send_message_number;
	status = ratchet_send(
			conversation->ratchet,
			send_header_key,
			&send_message_number,
			&previous_send_message_number,
			send_ephemeral,
			send_message_key);
	if (status != 0) {
		goto cleanup;
	}

	//create the header
	status = header_construct(
			header,
			send_ephemeral,
			send_message_number,
			previous_send_message_number);
	if (status != 0) {
		goto cleanup;
	}

	//now encrypt the message with those keys
	const size_t packet_length = header->content_length + 3 + HEADER_NONCE_SIZE + MESSAGE_NONCE_SIZE + crypto_aead_chacha20poly1305_ABYTES + crypto_secretbox_MACBYTES + message->content_length + 255;
	*packet = buffer_create_on_heap(packet_length, 0);
	status = packet_encrypt(
			*packet,
			PREKEY_MESSAGE,
			0, //current protocol version
			0, //highest supported protocol version
			header,
			send_header_key,
			message,
			send_message_key);
	if (status != 0) {
		buffer_destroy_from_heap(*packet);
		*packet = NULL;
		goto cleanup;
	}

cleanup:
	buffer_destroy_from_heap(send_header_key);
	buffer_destroy_from_heap(send_message_key);
	buffer_destroy_from_heap(send_ephemeral);
	buffer_destroy_from_heap(header);

	return status;
}
