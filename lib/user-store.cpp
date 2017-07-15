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

#include <algorithm>
#include <cassert>

#include "constants.h"
#include "user-store.h"

//create a new user_store
return_status user_store_create(user_store ** const store) {
	return_status status = return_status_init();

	//check input
	if (store == nullptr) {
		THROW(INVALID_INPUT, "Pointer to put new user store into is nullptr.");
	}

	*store = (user_store*)sodium_malloc(sizeof(user_store));
	THROW_on_failed_alloc(*store);

	//initialise
	(*store)->length = 0;
	(*store)->head = nullptr;
	(*store)->tail = nullptr;

cleanup:
	on_error {
		if (store != nullptr) {
			*store = nullptr;
		}
	}

	return status;
}

//destroy a user store
void user_store_destroy(user_store* store) {
	if (store != nullptr) {
		user_store_clear(store);
		sodium_free_and_null_if_valid(store);
	}
}

/*
 * add a new user node to a user store.
 */
static void add_user_store_node(user_store * const store, user_store_node * const node) {
	if ((store == nullptr) || (node == nullptr)) {
		return;
	}

	if (store->length == 0) { //first node in the list
		node->previous = nullptr;
		node->next = nullptr;
		store->head = node;
		store->tail = node;

		//update length
		store->length++;

		return;
	}

	//add the new node to the tail of the list
	store->tail->next = node;
	node->previous = store->tail;
	node->next = nullptr;
	store->tail = node;

	//update length
	store->length++;
}

/*
 * create an empty user_store_node and set up all the pointers.
 */
static return_status create_user_store_node(user_store_node ** const node) {
	return_status status = return_status_init();

	if (node == nullptr) {
		THROW(INVALID_INPUT, "Pointer to put new user store node into is nullptr.");
	}

	*node = (user_store_node*)sodium_malloc(sizeof(user_store_node));
	THROW_on_failed_alloc(*node);

	//initialise pointers
	(*node)->previous = nullptr;
	(*node)->next = nullptr;
	(*node)->prekeys = nullptr;
	(*node)->master_keys = nullptr;

	//initialise the public_signing key buffer
	(*node)->public_signing_key->init((*node)->public_signing_key_storage, PUBLIC_MASTER_KEY_SIZE, PUBLIC_MASTER_KEY_SIZE);

	conversation_store_init((*node)->conversations);

cleanup:
	on_error {
		if (node != nullptr) {
			*node = nullptr;
		}
	}

	return status;
}

/*
 * Create a new user and add it to the user store.
 *
 * The seed is optional an can be used to add entropy in addition
 * to the entropy provided by the OS. IMPORTANT: Don't put entropy in
 * here, that was generated by the OSs CPRNG!
 */
return_status user_store_create_user(
		user_store *store,
		Buffer * const seed, //optional, can be nullptr
		Buffer * const public_signing_key, //output, optional, can be nullptr
		Buffer * const public_identity_key) { //output, optional, can be nullptr

	return_status status = return_status_init();

	user_store_node *new_node = nullptr;
	status = create_user_store_node(&new_node);
	THROW_on_error(CREATION_ERROR, "Failed to create new user store node.");

	//generate the master keys
	status = master_keys_create(
			&(new_node->master_keys),
			seed,
			new_node->public_signing_key,
			public_identity_key);
	THROW_on_error(CREATION_ERROR, "Failed to create master keys.");

	//prekeys
	status = prekey_store_create(&(new_node->prekeys));
	THROW_on_error(CREATION_ERROR, "Failed to create prekey store.")

	//copy the public signing key, if requested
	if (public_signing_key != nullptr) {
		if (public_signing_key->getBufferLength() < PUBLIC_MASTER_KEY_SIZE) {
			THROW(INCORRECT_BUFFER_SIZE, "Invalidly sized buffer for public signing key.");
		}

		if (public_signing_key->cloneFrom(new_node->public_signing_key) != 0) {
			THROW(BUFFER_ERROR, "Failed to clone public signing key.");
		}
	}

	add_user_store_node(store, new_node);

cleanup:
	on_error {
		if (new_node != nullptr) {
			if (new_node->prekeys != nullptr) {
				prekey_store_destroy(new_node->prekeys);
			}
			if (new_node->master_keys != nullptr) {
				sodium_free_and_null_if_valid(new_node->master_keys);
			}

			sodium_free_and_null_if_valid(new_node);
		}
	}

	return status;
}

/*
 * Find a user for a given public signing key.
 *
 * Returns nullptr if no user was found.
 */
return_status user_store_find_node(user_store_node ** const node, user_store * const store, Buffer * const public_signing_key) {
	return_status status = return_status_init();

	if ((node == nullptr) || (public_signing_key == nullptr) || (public_signing_key->content_length != PUBLIC_MASTER_KEY_SIZE)) {
		THROW(INVALID_INPUT, "Invalid input for user_store_find_node.");
	}

	*node = store->head;

	//search for the matching public identity key
	while (*node != nullptr) {
		if ((*node)->public_signing_key->compare(public_signing_key) == 0) {
			//match found
			break;
		}
		*node = (*node)->next; //go on through the list
	}
	if (*node == nullptr) {
		THROW(NOT_FOUND, "Couldn't find the user store node.");
	}

cleanup:
	on_error {
		if (node != nullptr) {
			*node = nullptr;
		}
	}

	return status;
}

/*
 * List all of the users.
 *
 * Returns a buffer containing a list of all the public
 * signing keys of the user.
 *
 * The buffer is heap allocated, so don't forget to free it!
 */
return_status user_store_list(Buffer ** const list, user_store * const store) {
	return_status status = return_status_init();

	if ((list == nullptr) || (store == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to user_store_list.");
	}

	*list = Buffer::create(PUBLIC_MASTER_KEY_SIZE * store->length, PUBLIC_MASTER_KEY_SIZE * store->length);
	THROW_on_failed_alloc(*list);

	{
		user_store_node *current_node = store->head;
		for (size_t i = 0; (i < store->length) && (current_node != nullptr); i++) {
			int status_int = (*list)->copyFrom(
					i * PUBLIC_MASTER_KEY_SIZE,
					current_node->public_signing_key,
					0,
					current_node->public_signing_key->content_length);
			if (status_int != 0) { //copying went wrong
				THROW(BUFFER_ERROR, "Failed to copy public master key to user list.");
			}

			user_store_node *next_node = current_node->next;
			current_node = next_node;
		}
	}

cleanup:
	on_error {
		if (list != nullptr) {
				buffer_destroy_from_heap_and_null_if_valid(*list);
		}
	}

	return status;
}

/*
 * Remove a user from the user store.
 *
 * The user is identified by it's public signing key.
 */
return_status user_store_remove_by_key(user_store * const store, Buffer * const public_signing_key) {
	return_status status = return_status_init();

	user_store_node *node = nullptr;
	status = user_store_find_node(&node, store, public_signing_key);
	THROW_on_error(NOT_FOUND, "Failed to find user to remove.");

	user_store_remove(store, node);

cleanup:
	return status;
}

//remove a user form the user store
void user_store_remove(user_store *store, user_store_node *node) {
	if (node == nullptr) {
		return;
	}

	//clear the conversation store
	conversation_store_clear(node->conversations);

	if (node->next != nullptr) { //node is not the tail
		node->next->previous = node->previous;
	} else { //node ist the tail
		store->tail = node->previous;
	}
	if (node->previous != nullptr) { //node ist not the head
		node->previous->next = node->next;
	} else { //node is the head
		store->head = node->next;
	}

	sodium_free_and_null_if_valid(node);

	//update length
	store->length--;
}

//clear the entire user store
void user_store_clear(user_store *store){
	if (store == nullptr) {
		return;
	}

	while (store->length > 0) {
		user_store_remove(store, store->head);
	}

}

return_status user_store_node_export(user_store_node * const node, User ** const user) __attribute__((warn_unused_result));
return_status user_store_node_export(user_store_node * const node, User ** const user) {
	return_status status = return_status_init();

	//master keys
	Key *public_signing_key = nullptr;
	Key *private_signing_key = nullptr;
	Key *public_identity_key = nullptr;
	Key *private_identity_key = nullptr;

	//conversation store
	Conversation **conversations = nullptr;
	size_t conversations_length = 0;

	//prekeys
	Prekey **prekeys = nullptr;
	size_t prekeys_length = 0;
	Prekey **deprecated_prekeys = nullptr;
	size_t deprecated_prekeys_length = 0;

	//check input
	if ((node == nullptr) || (user == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to user_store_node_export.");
	}

	*user = (User*)zeroed_malloc(sizeof(User));
	THROW_on_failed_alloc(*user);
	user__init(*user);

	//export master keys
	status = master_keys_export(
		node->master_keys,
		&public_signing_key,
		&private_signing_key,
		&public_identity_key,
		&private_identity_key);
	THROW_on_error(EXPORT_ERROR, "Failed to export masters keys.");

	(*user)->public_signing_key = public_signing_key;
	public_signing_key = nullptr;
	(*user)->private_signing_key = private_signing_key;
	private_signing_key = nullptr;
	(*user)->public_identity_key = public_identity_key;
	public_identity_key = nullptr;
	(*user)->private_identity_key = private_identity_key;
	private_identity_key = nullptr;

	//export the conversation store
	status = conversation_store_export(node->conversations, &conversations, &conversations_length);
	THROW_on_error(EXPORT_ERROR, "Failed to export conversation store.");

	(*user)->conversations = conversations;
	conversations = nullptr;
	(*user)->n_conversations = conversations_length;
	conversations_length = 0;

	//export the prekeys
	status = prekey_store_export(
		node->prekeys,
		&prekeys,
		&prekeys_length,
		&deprecated_prekeys,
		&deprecated_prekeys_length);
	THROW_on_error(EXPORT_ERROR, "Failed to export prekeys.");

	(*user)->prekeys = prekeys;
	prekeys = nullptr;
	(*user)->n_prekeys = prekeys_length;
	prekeys_length = 0;
	(*user)->deprecated_prekeys = deprecated_prekeys;
	deprecated_prekeys = nullptr;
	(*user)->n_deprecated_prekeys = deprecated_prekeys_length;
	deprecated_prekeys_length = 0;

cleanup:
	on_error {
		if (user != nullptr) {
			user__free_unpacked(*user, &protobuf_c_allocators);
			*user = nullptr;
		}
	}

	return status;
}

return_status user_store_export(
		const user_store * const store,
		User *** const users,
		size_t * const users_length) {
	return_status status = return_status_init();

	//check input
	if ((users == nullptr) || (users == nullptr) || (users_length == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to user_store_export.");
	}

	if (store->length > 0) {
		*users = (User**)zeroed_malloc(store->length * sizeof(User*));
		THROW_on_failed_alloc(*users);
		std::fill(*users, *users + store->length, nullptr);

		size_t i = 0;
		user_store_node *node = nullptr;
		for (i = 0, node = store->head; (i < store->length) && (node != nullptr); i++, node = node->next) {
			status = user_store_node_export(node, &((*users)[i]));
			THROW_on_error(EXPORT_ERROR, "Failed to export user store node.");
		}
	} else {
		*users = nullptr;
	}

	*users_length = store->length;
cleanup:
	on_error {
		if ((users != nullptr) && (*users != nullptr) && (users_length != 0)) {
			for (size_t i = 0; i < *users_length; i++) {
				user__free_unpacked((*users)[i], &protobuf_c_allocators);
				(*users)[i] = nullptr;
			}
			zeroed_free_and_null_if_valid(*users);
		}
	}

	return status;
}

static return_status user_store_node_import(user_store_node ** const node, const User * const user) {
	return_status status = return_status_init();

	//check input
	if ((node == nullptr) || (user == nullptr)) {
		THROW(INVALID_INPUT, "Invalid input to user_store_node_import.");
	}

	status = create_user_store_node(node);
	THROW_on_error(CREATION_ERROR, "Failed to create user store node.");

	//master keys
	status = master_keys_import(
		&((*node)->master_keys),
		user->public_signing_key,
		user->private_signing_key,
		user->public_identity_key,
		user->private_identity_key);
	THROW_on_error(IMPORT_ERROR, "Failed to import master keys.");

	//public signing key
	if (user->public_signing_key == nullptr) {
		THROW(PROTOBUF_MISSING_ERROR, "Missing public signing key in Protobuf-C struct.");
	}
	if ((*node)->public_signing_key->cloneFromRaw(user->public_signing_key->key.data, user->public_signing_key->key.len) != 0) {
		THROW(BUFFER_ERROR, "Failed to copy public signing key.");
	}

	status = conversation_store_import(
		(*node)->conversations,
		user->conversations,
		user->n_conversations);
	THROW_on_error(IMPORT_ERROR, "Failed to import conversations.");

	status = prekey_store_import(
		&((*node)->prekeys),
		user->prekeys,
		user->n_prekeys,
		user->deprecated_prekeys,
		user->n_deprecated_prekeys);
	THROW_on_error(IMPORT_ERROR, "Failed to import prekeys.");

cleanup:
	return status;
}

return_status user_store_import(
		user_store ** const store,
		User ** users,
		const size_t users_length) {
	return_status status = return_status_init();

	//check input
	if ((store == nullptr)
			|| ((users_length == 0) && (users != nullptr))
			|| ((users_length > 0) && (users == nullptr))) {
		THROW(INVALID_INPUT, "Invalid input to user_store_import.");
	}

	status = user_store_create(store);
	THROW_on_error(CREATION_ERROR, "Failed to create user store.");

	{
		size_t i = 0;
		user_store_node *node = nullptr;
		for (i = 0; i < users_length; i++) {
			status = user_store_node_import(&node, users[i]);
			THROW_on_error(IMPORT_ERROR, "Failed to import user store node.");

			add_user_store_node(*store, node);
		}
	}

cleanup:
	on_error {
		if (store != nullptr) {
			user_store_destroy(*store);
		}
	}

	return status;
}

