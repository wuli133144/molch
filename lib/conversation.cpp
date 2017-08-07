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
			|| (our_private_identity == nullptr) || !our_private_identity->contains(PRIVATE_KEY_SIZE)
			|| (our_public_identity == nullptr) || !our_public_identity->contains(PUBLIC_KEY_SIZE)
			|| (their_public_identity == nullptr) || !their_public_identity->contains(PUBLIC_KEY_SIZE)
			|| (our_private_ephemeral == nullptr) || !our_public_ephemeral->contains(PRIVATE_KEY_SIZE)
			|| (our_public_ephemeral == nullptr) || !our_public_ephemeral->contains(PUBLIC_KEY_SIZE)
			|| (their_public_ephemeral == nullptr) || !their_public_ephemeral->contains(PUBLIC_KEY_SIZE)) {
		THROW(INVALID_INPUT, "Invalid input for conversation_create.");
	}

	*conversation = reinterpret_cast<conversation_t*>(malloc(sizeof(conversation_t)));
	THROW_on_failed_alloc(*conversation);

	init_struct(*conversation);

	//create random id
	if ((*conversation)->id.fillRandom(CONVERSATION_ID_SIZE) != 0) {
		THROW(BUFFER_ERROR, "Failed to create random conversation id.");
	}

	try {
		(*conversation)->ratchet = new Ratchet(
				*our_private_identity,
				*our_public_identity,
				*their_public_identity,
				*our_private_ephemeral,
				*our_public_ephemeral,
				*their_public_ephemeral);
		THROW_on_error(CREATION_ERROR, "Failed to create ratchet.");
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}

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
	delete conversation->ratchet;
	if (conversation != nullptr) {
		free(conversation);
	}
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
		std::unique_ptr<Buffer>& packet, //output
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
			|| (receiver_public_identity == nullptr) || !receiver_public_identity->contains(PUBLIC_KEY_SIZE)
			|| (sender_public_identity == nullptr) || !sender_public_identity->contains(PUBLIC_KEY_SIZE)
			|| (sender_private_identity == nullptr) || !sender_private_identity->contains(PRIVATE_KEY_SIZE)
			|| (receiver_prekey_list == nullptr) || !receiver_prekey_list->contains((PREKEY_AMOUNT * PUBLIC_KEY_SIZE))) {
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
		std::unique_ptr<Buffer>& message, //output
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
			|| (receiver_public_identity == nullptr) || !receiver_public_identity->contains(PUBLIC_KEY_SIZE)
			|| (receiver_private_identity == nullptr) || !receiver_private_identity->contains(PRIVATE_KEY_SIZE)
			|| (receiver_prekeys == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to conversation_start_receive_conversation.");
	}

	*conversation = nullptr;

	//get the senders keys and our public prekey from the packet
	molch_message_type packet_type;
	uint32_t current_protocol_version;
	uint32_t highest_supported_protocol_version;
	try {
		packet_get_metadata_without_verification(
			current_protocol_version,
			highest_supported_protocol_version,
			packet_type,
			*packet,
			&sender_public_identity,
			&sender_public_ephemeral,
			&receiver_public_prekey);
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}

	if (packet_type != PREKEY_MESSAGE) {
		THROW(INVALID_VALUE, "Packet is not a prekey message.");
	}

	//get the private prekey that corresponds to the public prekey used in the message
	try {
		receiver_prekeys->getPrekey(receiver_public_prekey, receiver_private_prekey);
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}

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
		std::unique_ptr<Buffer>& packet, //output
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
	if ((conversation == nullptr) || (message == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to conversation_send.");
	}

	//ensure that either both public keys are nullptr or set
	if (((public_identity_key == nullptr) && (public_prekey != nullptr)) || ((public_prekey == nullptr) && (public_identity_key != nullptr))) {
		THROW(INVALID_INPUT, "Invalid combination of provided key buffers.");
	}

	//check the size of the public keys
	if (((public_identity_key != nullptr) && !public_identity_key->contains(PUBLIC_KEY_SIZE)) || ((public_prekey != nullptr) && !public_prekey->contains(PUBLIC_KEY_SIZE))) {
		THROW(INCORRECT_BUFFER_SIZE, "Public key output has incorrect size.");
	}

	packet_type = NORMAL_MESSAGE;
	//check if this is a prekey message
	if (public_identity_key != nullptr) {
		packet_type = PREKEY_MESSAGE;
	}

	uint32_t send_message_number;
	uint32_t previous_send_message_number;
	try {
		conversation->ratchet->send(
				send_header_key,
				send_message_number,
				previous_send_message_number,
				send_ephemeral_key,
				send_message_key);
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}
	THROW_on_error(SEND_ERROR, "Failed to get send keys.");

	try {
		header = header_construct(
				send_ephemeral_key,
				send_message_number,
				previous_send_message_number);

		packet = packet_encrypt(
				packet_type,
				*header,
				send_header_key,
				*message,
				send_message_key,
				public_identity_key,
				public_ephemeral_key,
				public_prekey);
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}

cleanup:
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
		HeaderAndMessageKeyStore& skipped_keys,
		const Buffer& packet,
		std::unique_ptr<Buffer>& message,
		uint32_t& receive_message_number,
		uint32_t& previous_receive_message_number) {
	//create buffers
	std::unique_ptr<Buffer> header;
	Buffer their_signed_public_ephemeral(PUBLIC_KEY_SIZE, PUBLIC_KEY_SIZE);
	exception_on_invalid_buffer(their_signed_public_ephemeral);

	for (size_t index = 0; index < skipped_keys.keys.size(); index++) {
		HeaderAndMessageKeyStoreNode& node = skipped_keys.keys[index];
		bool decryption_successful = true;
		try {
			header = packet_decrypt_header(packet, node.header_key);
		} catch (const MolchException& exception) {
			decryption_successful = false;
		}
		if (decryption_successful) {
			try {
				message = packet_decrypt_message(packet, node.message_key);
			} catch (const MolchException& exception) {
				decryption_successful = false;
			}
			if (decryption_successful) {
				skipped_keys.keys.erase(skipped_keys.keys.cbegin() + static_cast<ptrdiff_t>(index));
				index--;

				header_extract(
						their_signed_public_ephemeral,
						receive_message_number,
						previous_receive_message_number,
						*header);
				return SUCCESS;
			}
		}
	}

	return NOT_FOUND;
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
	std::unique_ptr<Buffer>& message) noexcept { //output, free after use!
	return_status status = return_status_init();

	bool decryptable = true;

	//create buffers
	std::unique_ptr<Buffer> header;

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
			|| (receive_message_number == nullptr)
			|| (previous_receive_message_number == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to conversation_receive.");
	}

	try {
		int status = try_skipped_header_and_message_keys(
				conversation->ratchet->skipped_header_and_message_keys,
				*packet,
				message,
				*receive_message_number,
				*previous_receive_message_number);
		if (status == SUCCESS) {
			// found a key and successfully decrypted the message
			goto cleanup;
		}

		conversation->ratchet->getReceiveHeaderKeys(current_receive_header_key, next_receive_header_key);
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}

	//try to decrypt the packet header with the current receive header key
	try {
		header = packet_decrypt_header(*packet, current_receive_header_key);
	} catch (const MolchException& exception) {
		decryptable = false;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}
	if (decryptable) {
		try {
			conversation->ratchet->setHeaderDecryptability(CURRENT_DECRYPTABLE);
		} catch (const MolchException& exception) {
			status = exception.toReturnStatus();
			goto cleanup;
		} catch (const std::exception& exception) {
			THROW(EXCEPTION, exception.what());
		}
	} else {
		return_status_destroy_errors(&status); //free the error stack to avoid memory leak.

		//since this failed, try to decrypt it with the next receive header key
		decryptable = true;
		try {
			header = packet_decrypt_header(*packet, next_receive_header_key);
		} catch (const MolchException& exception) {
			decryptable = false;
		} catch (const std::exception& exception) {
			THROW(EXCEPTION, exception.what());
		}
		if (decryptable) {
			try {
				conversation->ratchet->setHeaderDecryptability(NEXT_DECRYPTABLE);
			} catch (const MolchException& exception) {
				status = exception.toReturnStatus();
				goto cleanup;
			} catch (const std::exception& exception) {
				THROW(EXCEPTION, exception.what());
			}
		} else {
			try {
				conversation->ratchet->setHeaderDecryptability(UNDECRYPTABLE);
			} catch (...) {
			}
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
	try {
		conversation->ratchet->receive(
			message_key,
			their_signed_public_ephemeral,
			local_receive_message_number,
			local_previous_receive_message_number);
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}
	THROW_on_error(DECRYPT_ERROR, "Failed to get decryption keys.");

	try {
		message = packet_decrypt_message(*packet, message_key);
	} catch (...) {
		try {
			conversation->ratchet->setLastMessageAuthenticity(false);
		} catch (...) {
		}
		THROW(DECRYPT_ERROR, "Failed to decrypt message.");
	}

	try {
		conversation->ratchet->setLastMessageAuthenticity(true);
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}

	*receive_message_number = local_receive_message_number;
	*previous_receive_message_number = local_previous_receive_message_number;

cleanup:
	on_error {
		if (conversation != nullptr) {
			try {
				conversation->ratchet->setLastMessageAuthenticity(false);
			} catch (...) {
			}
		}
	}

	return status;
}

return_status conversation_export(
		conversation_t * const conversation,
		Conversation ** const exported_conversation) noexcept {
	return_status status = return_status_init();

	std::unique_ptr<Conversation,ConversationDeleter> exported_conversation_ptr;
	unsigned char *id = nullptr;

	//check input
	if ((conversation == nullptr) || (exported_conversation == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to conversation_export.");
	}

	//export the ratchet
	try {
		exported_conversation_ptr = conversation->ratchet->exportProtobuf();
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}

	//export the conversation id
	id = reinterpret_cast<unsigned char*>(zeroed_malloc(CONVERSATION_ID_SIZE));
	THROW_on_failed_alloc(id);
	if (conversation->id.cloneToRaw(id, CONVERSATION_ID_SIZE) != 0) {
		THROW(BUFFER_ERROR, "Failed to copy conversation id.");
	}
	exported_conversation_ptr->id.data = id;
	exported_conversation_ptr->id.len = CONVERSATION_ID_SIZE;
	*exported_conversation = exported_conversation_ptr.release();

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
	*conversation = reinterpret_cast<conversation_t*>(malloc(sizeof(conversation_t)));
	THROW_on_failed_alloc(*conversation);
	init_struct(*conversation);

	//copy the id
	if ((*conversation)->id.cloneFromRaw(conversation_protobuf->id.data, conversation_protobuf->id.len) != 0) {
		THROW(BUFFER_ERROR, "Failed to copy id.");
	}

	//import the ratchet
	try {
		(*conversation)->ratchet = new Ratchet(*conversation_protobuf);
	} catch (const MolchException& exception) {
		status = exception.toReturnStatus();
		goto cleanup;
	} catch (const std::exception& exception) {
		THROW(EXCEPTION, exception.what());
	}

cleanup:
	on_error {
		if (conversation != nullptr) {
			free_and_null_if_valid(*conversation);
		}
	}
	return status;
}

