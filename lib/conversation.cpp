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

#include <exception>

#include "constants.h"
#include "conversation.h"
#include "molch.h"
#include "packet.h"
#include "header.h"
#include "molch-exception.h"

/*
 * Create a new conversation struct and initialise the buffer pointer.
 */
static void init_struct(conversation_t *conversation) noexcept {
	conversation->id.init(conversation->id_storage, CONVERSATION_ID_SIZE, CONVERSATION_ID_SIZE);
	conversation->ratchet = nullptr;
	conversation->previous = nullptr;
	conversation->next = nullptr;
}

/*
 * Create a new conversation.
 *
 * Don't forget to destroy the return status with return_status_destroy_errors()
 * if an error has occurred.
 */
return_status conversation_create(
		conversation_t **const conversation,
		Buffer * const,
		Buffer * const,
		Buffer * const,
		Buffer * const,
		Buffer * const,
		Buffer * const) noexcept __attribute__((warn_unused_result));
return_status conversation_create(
		conversation_t **const conversation,
		Buffer * const our_private_identity,
		Buffer * const our_public_identity,
		Buffer * const their_public_identity,
		Buffer * const our_private_ephemeral,
		Buffer * const our_public_ephemeral,
		Buffer * const their_public_ephemeral) noexcept {

	return_status status = return_status_init();

	//check input
	if ((conversation == nullptr)
			|| (our_private_identity == nullptr) || (our_private_identity->content_length != PRIVATE_KEY_SIZE)
			|| (our_public_identity == nullptr) || (our_public_identity->content_length != PUBLIC_KEY_SIZE)
			|| (their_public_identity == nullptr) || (their_public_identity->content_length != PUBLIC_KEY_SIZE)
			|| (our_private_ephemeral == nullptr) || (our_public_ephemeral->content_length != PRIVATE_KEY_SIZE)
			|| (our_public_ephemeral == nullptr) || (our_public_ephemeral->content_length != PUBLIC_KEY_SIZE)
			|| (their_public_ephemeral == nullptr) || (their_public_ephemeral->content_length != PUBLIC_KEY_SIZE)) {
		THROW(INVALID_INPUT, "Invalid input for conversation_create.");
	}

	*conversation = (conversation_t*)malloc(sizeof(conversation_t));
	THROW_on_failed_alloc(*conversation);

	init_struct(*conversation);

	//create random id
	if ((*conversation)->id.fillRandom(CONVERSATION_ID_SIZE) != 0) {
		THROW(BUFFER_ERROR, "Failed to create random conversation id.");
	}

	status = Ratchet::create(
			(*conversation)->ratchet,
			*our_private_identity,
			*our_public_identity,
			*their_public_identity,
			*our_private_ephemeral,
			*our_public_ephemeral,
			*their_public_ephemeral);
	THROW_on_error(CREATION_ERROR, "Failed to create ratchet.");

cleanup:
	on_error {
		if (conversation != nullptr) {
			free_and_null_if_valid(*conversation);
		}
	}

	return status;
}

/*
 * Destroy a conversation.
 */
void conversation_destroy(conversation_t * const conversation) noexcept {
	if (conversation->ratchet != nullptr) {
		conversation->ratchet->destroy();
	}
	free(conversation);
}

/*
 * Start a new conversation where we are the sender.
 *
 * Don't forget to destroy the return status with return_status_destroy_errors()
 * if an error has occurred.
 */
return_status conversation_start_send_conversation(
		conversation_t ** const conversation, //output, newly created conversation
		Buffer * const message, //message we want to send to the receiver
		Buffer ** packet, //output, free after use!
		Buffer * const sender_public_identity, //who is sending this message?
		Buffer * const sender_private_identity,
		Buffer * const receiver_public_identity,
		Buffer * const receiver_prekey_list //PREKEY_AMOUNT * PUBLIC_KEY_SIZE
		) noexcept {

	return_status status = return_status_init();

	uint32_t prekey_number;

	Buffer sender_public_ephemeral(PUBLIC_KEY_SIZE, PUBLIC_KEY_SIZE);
	Buffer sender_private_ephemeral(PRIVATE_KEY_SIZE, PRIVATE_KEY_SIZE);
	throw_on_invalid_buffer(sender_public_ephemeral);
	throw_on_invalid_buffer(sender_private_ephemeral);

	//check many error conditions
	if ((conversation == nullptr)
			|| (message == nullptr)
			|| (packet == nullptr)
			|| (receiver_public_identity == nullptr) || (receiver_public_identity->content_length != PUBLIC_KEY_SIZE)
			|| (sender_public_identity == nullptr) || (sender_public_identity->content_length != PUBLIC_KEY_SIZE)
			|| (sender_private_identity == nullptr) || (sender_private_identity->content_length != PRIVATE_KEY_SIZE)
			|| (receiver_prekey_list == nullptr) || (receiver_prekey_list->content_length != (PREKEY_AMOUNT * PUBLIC_KEY_SIZE))) {
		THROW(INVALID_INPUT, "Invalid input to conversation_start_send_conversation.");
	}

	*conversation = nullptr;

	{
		int status_int = 0;
		//create an ephemeral keypair
		status_int = crypto_box_keypair(sender_public_ephemeral.content, sender_private_ephemeral.content);
		if (status_int != 0) {
			THROW(KEYGENERATION_FAILED, "Failed to generate ephemeral keypair.");
		}
	}

	//choose a prekey
	prekey_number = randombytes_uniform(PREKEY_AMOUNT);
	{
		Buffer receiver_public_prekey(
				&(receiver_prekey_list->content[prekey_number * PUBLIC_KEY_SIZE]),
				PUBLIC_KEY_SIZE);

		//initialize the conversation
		status = conversation_create(
				conversation,
				sender_private_identity,
				sender_public_identity,
				receiver_public_identity,
				&sender_private_ephemeral,
				&sender_public_ephemeral,
				&receiver_public_prekey);
		THROW_on_error(CREATION_ERROR, "Failed to create conversation.");

		status = conversation_send(
				*conversation,
				message,
				packet,
				sender_public_identity,
				&sender_public_ephemeral,
				&receiver_public_prekey);
		THROW_on_error(SEND_ERROR, "Failed to send message using newly created conversation.");
	}

cleanup:
	on_error {
		if (conversation != nullptr) {
			if (*conversation != nullptr) {
				conversation_destroy(*conversation);
			}
			*conversation = nullptr;
		}
	}

	return status;
}

/*
 * Start a new conversation where we are the receiver.
 *
 * Don't forget to destroy the return status with return_status_destroy_errors()
 * if an error has occurred.
 */
return_status conversation_start_receive_conversation(
		conversation_t ** const conversation, //output, newly created conversation
		Buffer * const packet, //received packet
		Buffer ** message, //output, free after use!
		Buffer * const receiver_public_identity,
		Buffer * const receiver_private_identity,
		PrekeyStore * const receiver_prekeys //prekeys of the receiver
		) noexcept {
	uint32_t receive_message_number = 0;
	uint32_t previous_receive_message_number = 0;

	return_status status = return_status_init();

	//key buffers
	Buffer receiver_public_prekey(PUBLIC_KEY_SIZE, PUBLIC_KEY_SIZE);
	Buffer receiver_private_prekey(PRIVATE_KEY_SIZE, PRIVATE_KEY_SIZE);
	Buffer sender_public_ephemeral(PUBLIC_KEY_SIZE, PUBLIC_KEY_SIZE);
	Buffer sender_public_identity(PUBLIC_KEY_SIZE, PUBLIC_KEY_SIZE);
	throw_on_invalid_buffer(receiver_public_prekey);
	throw_on_invalid_buffer(receiver_private_prekey);
	throw_on_invalid_buffer(sender_public_ephemeral);
	throw_on_invalid_buffer(sender_public_identity);

	if ((conversation == nullptr)
			|| (packet ==nullptr)
			|| (message == nullptr)
			|| (receiver_public_identity == nullptr) || (receiver_public_identity->content_length != PUBLIC_KEY_SIZE)
			|| (receiver_private_identity == nullptr) || (receiver_private_identity->content_length != PRIVATE_KEY_SIZE)
			|| (receiver_prekeys == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to conversation_start_receive_conversation.");
	}

	*conversation = nullptr;

	//get the senders keys and our public prekey from the packet
	molch_message_type packet_type;
	uint32_t current_protocol_version;
	uint32_t highest_supported_protocol_version;
	status = packet_get_metadata_without_verification(
			current_protocol_version,
			highest_supported_protocol_version,
			packet_type,
			*packet,
			&sender_public_identity,
			&sender_public_ephemeral,
			&receiver_public_prekey);
	THROW_on_error(GENERIC_ERROR, "Failed to get packet metadata.");

	if (packet_type != PREKEY_MESSAGE) {
		THROW(INVALID_VALUE, "Packet is not a prekey message.");
	}

	//get the private prekey that corresponds to the public prekey used in the message
	status = receiver_prekeys->getPrekey(
			receiver_public_prekey,
			receiver_private_prekey);
	THROW_on_error(DATA_FETCH_ERROR, "Failed to get public prekey.");

	status = conversation_create(
			conversation,
			receiver_private_identity,
			receiver_public_identity,
			&sender_public_identity,
			&receiver_private_prekey,
			&receiver_public_prekey,
			&sender_public_ephemeral);
	THROW_on_error(CREATION_ERROR, "Failed to create conversation.");

	status = conversation_receive(
			*conversation,
			packet,
			&receive_message_number,
			&previous_receive_message_number,
			message);
	THROW_on_error(RECEIVE_ERROR, "Failed to receive message.");

cleanup:
	on_error {
		if (conversation != nullptr) {
			if (*conversation != nullptr) {
				conversation_destroy(*conversation);
			}
			*conversation = nullptr;
		}
	}

	return status;
}

/*
 * Send a message using an existing conversation.
 *
 * Don't forget to destroy the return status with return_status_destroy_errors()
 * if an error has occurred.
 */
return_status conversation_send(
		conversation_t * const conversation,
		Buffer * const message,
		Buffer **packet, //output, free after use!
		Buffer * const public_identity_key, //can be nullptr, if not nullptr, this will be a prekey message
		Buffer * const public_ephemeral_key, //can be nullptr, if not nullptr, this will be a prekey message
		Buffer * const public_prekey //can be nullptr, if not nullptr, this will be a prekey message
		) noexcept {
	return_status status = return_status_init();

	molch_message_type packet_type;

	//create the header
	std::unique_ptr<Buffer> header;

	Buffer send_header_key(HEADER_KEY_SIZE, HEADER_KEY_SIZE);
	Buffer send_message_key(MESSAGE_KEY_SIZE, MESSAGE_KEY_SIZE);
	Buffer send_ephemeral_key(PUBLIC_KEY_SIZE, 0);
	throw_on_invalid_buffer(send_header_key);
	throw_on_invalid_buffer(send_message_key);
	throw_on_invalid_buffer(send_ephemeral_key);


	//check input
	if ((conversation == nullptr)
			|| (message == nullptr)
			|| (packet == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to conversation_send.");
	}

	//ensure that either both public keys are nullptr or set
	if (((public_identity_key == nullptr) && (public_prekey != nullptr)) || ((public_prekey == nullptr) && (public_identity_key != nullptr))) {
		THROW(INVALID_INPUT, "Invalid combination of provided key buffers.");
	}

	//check the size of the public keys
	if (((public_identity_key != nullptr) && (public_identity_key->content_length != PUBLIC_KEY_SIZE)) || ((public_prekey != nullptr) && (public_prekey->content_length != PUBLIC_KEY_SIZE))) {
		THROW(INCORRECT_BUFFER_SIZE, "Public key output has incorrect size.");
	}

	packet_type = NORMAL_MESSAGE;
	//check if this is a prekey message
	if (public_identity_key != nullptr) {
		packet_type = PREKEY_MESSAGE;
	}

	*packet = nullptr;

	uint32_t send_message_number;
	uint32_t previous_send_message_number;
	status = conversation->ratchet->send(
			send_header_key,
			send_message_number,
			previous_send_message_number,
			send_ephemeral_key,
			send_message_key);
	THROW_on_error(SEND_ERROR, "Failed to get send keys.");

	try {
		header = header_construct(
				send_ephemeral_key,
				send_message_number,
				previous_send_message_number);
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}

	status = packet_encrypt(
			*packet,
			packet_type,
			*header,
			send_header_key,
			*message,
			send_message_key,
			public_identity_key,
			public_ephemeral_key,
			public_prekey);
	THROW_on_error(ENCRYPT_ERROR, "Failed to encrypt packet.");

cleanup:
	on_error {
		if (packet != nullptr) {
			buffer_destroy_and_null_if_valid(*packet);
		}
	}

	return status;
}

/*
 * Try to decrypt a packet with skipped over header and message keys.
 * This corresponds to "try_skipped_header_and_message_keys" from the
 * Axolotl protocol description.
 *
 * Returns 0, if it was able to decrypt the packet.
 */
static int try_skipped_header_and_message_keys(
		header_and_message_keystore * const skipped_keys,
		Buffer * const packet,
		Buffer ** const message,
		uint32_t * const receive_message_number,
		uint32_t * const previous_receive_message_number) noexcept {
	return_status status = return_status_init();

	//create buffers
	Buffer *header = nullptr;
	Buffer their_signed_public_ephemeral(PUBLIC_KEY_SIZE, PUBLIC_KEY_SIZE);
	throw_on_invalid_buffer(their_signed_public_ephemeral);

	{
		header_and_message_keystore_node* node = skipped_keys->head;
		for (size_t i = 0; (i < skipped_keys->length) && (node != nullptr); i++, node = node->next) {
			status = packet_decrypt_header(
					header,
					*packet,
					*node->header_key);
			if (status.status == SUCCESS) {
				status = packet_decrypt_message(
						*message,
						*packet,
						*node->message_key);
				if (status.status == SUCCESS) {
					header_and_message_keystore_remove(skipped_keys, node);

					try {
						header_extract(
								their_signed_public_ephemeral,
								*receive_message_number,
								*previous_receive_message_number,
								*header);
					} catch (const MolchException& exception) {
						status = exception.toReturnStatus();
						goto cleanup;
					} catch (const std::exception& exception) {
						THROW(EXCEPTION, exception.what());
					}
					THROW_on_error(GENERIC_ERROR, "Failed to extract data from header.");

					goto cleanup;
				}
			}
			return_status_destroy_errors(&status);
		}
	}

	status.status = NOT_FOUND;

cleanup:
	buffer_destroy_and_null_if_valid(header);

	on_error {
		if (message != nullptr) {
			buffer_destroy_and_null_if_valid(*message);
		}
	}

	return_status_destroy_errors(&status);

	return status.status;
}

/*
 * Receive and decrypt a message using an existing conversation.
 *
 * Don't forget to destroy the return status with return_status_destroy_errors()
 * if an error has occurred.
 */
return_status conversation_receive(
	conversation_t * const conversation,
	Buffer * const packet, //received packet
	uint32_t * const receive_message_number,
	uint32_t * const previous_receive_message_number,
	Buffer ** const message) noexcept { //output, free after use!
	return_status status = return_status_init();

	//create buffers
	Buffer *header = nullptr;

	Buffer current_receive_header_key(HEADER_KEY_SIZE, HEADER_KEY_SIZE);
	Buffer next_receive_header_key(HEADER_KEY_SIZE, HEADER_KEY_SIZE);
	Buffer their_signed_public_ephemeral(PUBLIC_KEY_SIZE, PUBLIC_KEY_SIZE);
	Buffer message_key(MESSAGE_KEY_SIZE, MESSAGE_KEY_SIZE);
	throw_on_invalid_buffer(current_receive_header_key);
	throw_on_invalid_buffer(next_receive_header_key);
	throw_on_invalid_buffer(their_signed_public_ephemeral);
	throw_on_invalid_buffer(message_key);

	if ((conversation == nullptr)
			|| (packet == nullptr)
			|| (message == nullptr)
			|| (receive_message_number == nullptr)
			|| (previous_receive_message_number == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to conversation_receive.");
	}

	*message = Buffer::create(packet->content_length, 0);
	THROW_on_failed_alloc(*message);

	{
		int status_int = try_skipped_header_and_message_keys(
				&conversation->ratchet->skipped_header_and_message_keys,
				packet,
				message,
				receive_message_number,
				previous_receive_message_number);
		if (status_int == 0) {
			// found a key and successfully decrypted the message
			goto cleanup;
		}
	}

	status = conversation->ratchet->getReceiveHeaderKeys(current_receive_header_key, next_receive_header_key);
	THROW_on_error(DATA_FETCH_ERROR, "Failed to get receive header keys.");

	//try to decrypt the packet header with the current receive header key
	status = packet_decrypt_header(
			header,
			*packet,
			current_receive_header_key);
	if (status.status == SUCCESS) {
		status = conversation->ratchet->setHeaderDecryptability(CURRENT_DECRYPTABLE);
		THROW_on_error(DATA_SET_ERROR, "Failed to set decryptability to CURRENT_DECRYPTABLE.");
	} else {
		return_status_destroy_errors(&status); //free the error stack to avoid memory leak.

		//since this failed, try to decrypt it with the next receive header key
		status = packet_decrypt_header(
				header,
				*packet,
				next_receive_header_key);
		if (status.status == SUCCESS) {
			status = conversation->ratchet->setHeaderDecryptability(NEXT_DECRYPTABLE);
			THROW_on_error(DATA_SET_ERROR, "Failed to set decryptability to NEXT_DECRYPTABLE.");
		} else {
			return_status decryptability_status = return_status_init();
			decryptability_status = conversation->ratchet->setHeaderDecryptability(UNDECRYPTABLE);
			return_status_destroy_errors(&decryptability_status);
			THROW(DECRYPT_ERROR, "Header undecryptable.");
		}
	}

	//extract data from the header
	uint32_t local_receive_message_number;
	uint32_t local_previous_receive_message_number;
	try {
		header_extract(
				their_signed_public_ephemeral,
				local_receive_message_number,
				local_previous_receive_message_number,
				*header);
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}

	//and now decrypt the message with the message key
	//now we have all the data we need to advance the ratchet
	//so let's do that
	status = conversation->ratchet->receive(
			message_key,
			their_signed_public_ephemeral,
			local_receive_message_number,
			local_previous_receive_message_number);
	THROW_on_error(DECRYPT_ERROR, "Failed to get decryption keys.");

	status = packet_decrypt_message(
			*message,
			*packet,
			message_key);
	on_error {
		return_status authenticity_status = return_status_init();
		authenticity_status = conversation->ratchet->setLastMessageAuthenticity(false);
		return_status_destroy_errors(&authenticity_status);
		THROW(DECRYPT_ERROR, "Failed to decrypt message.");
	}

	status = conversation->ratchet->setLastMessageAuthenticity(true);
	THROW_on_error(DATA_SET_ERROR, "Failed to set message authenticity.");

	*receive_message_number = local_receive_message_number;
	*previous_receive_message_number = local_previous_receive_message_number;

cleanup:
	on_error {
		return_status authenticity_status = return_status_init();
		if (conversation != nullptr) {
			authenticity_status = conversation->ratchet->setLastMessageAuthenticity(false);
			return_status_destroy_errors(&authenticity_status);
		}
		if (message != nullptr) {
			buffer_destroy_and_null_if_valid(*message);
		}
	}

	buffer_destroy_and_null_if_valid(header);

	return status;
}

return_status conversation_export(
		conversation_t * const conversation,
		Conversation ** const exported_conversation) noexcept {
	return_status status = return_status_init();

	unsigned char *id = nullptr;

	//check input
	if ((conversation == nullptr) || (exported_conversation == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to conversation_export.");
	}

	//export the ratchet
	status = conversation->ratchet->exportRatchet(*exported_conversation);
	THROW_on_error(EXPORT_ERROR, "Failed to export ratchet.");

	//export the conversation id
	id = (unsigned char*)zeroed_malloc(CONVERSATION_ID_SIZE);
	THROW_on_failed_alloc(id);
	if (conversation->id.cloneToRaw(id, CONVERSATION_ID_SIZE) != 0) {
		THROW(BUFFER_ERROR, "Failed to copy conversation id.");
	}
	(*exported_conversation)->id.data = id;
	(*exported_conversation)->id.len = CONVERSATION_ID_SIZE;
cleanup:
	on_error {
		zeroed_free_and_null_if_valid(id);
		if ((exported_conversation != nullptr) && (*exported_conversation != nullptr)) {
			conversation__free_unpacked(*exported_conversation, &protobuf_c_allocators);
		}
	}

	return status;
}

return_status conversation_import(
		conversation_t ** const conversation,
		const Conversation * const conversation_protobuf) noexcept {
	return_status status = return_status_init();

	//check input
	if ((conversation == nullptr) || (conversation_protobuf == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to conversation_import.");
	}

	//create the conversation
	*conversation = (conversation_t*)malloc(sizeof(conversation_t));
	THROW_on_failed_alloc(*conversation);
	init_struct(*conversation);

	//copy the id
	if ((*conversation)->id.cloneFromRaw(conversation_protobuf->id.data, conversation_protobuf->id.len) != 0) {
		THROW(BUFFER_ERROR, "Failed to copy id.");
	}

	//import the ratchet
	status = Ratchet::import(((*conversation)->ratchet), *conversation_protobuf);
	THROW_on_error(IMPORT_ERROR, "Failed to import ratchet.");
cleanup:
	on_error {
		if (conversation != nullptr) {
			free_and_null_if_valid(*conversation);
		}
	}
	return status;
}

