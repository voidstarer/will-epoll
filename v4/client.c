#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "client.h"
#include "log.h"

struct _client *client_array[MAX_CLIENTS];

struct _client *new_client(int fd)
{
	struct _client *client = calloc(1, sizeof(struct _client));
	client->fd = fd;
	client->id = UNIDENTIFIED_CLIENT;
	return client;
}

struct _client_list {
	struct _client *client;
	struct _client_list *next;
};

/*
 * The reason that we mark the clients as invalid and not free them straightaway is that
 * there might be an event of stale connection lying in the epoll loop, and if we free
 * the client right away while handling another connection, we might end up in a segfault.
 * Therefore, over here we mark the client and free the entire list at the end of loop.
 */
static struct _client_list *invalid_client_list;

void
mark_client_as_invalid(struct _client *client)
{
	struct _client_list *next;
	/* mark the client as invalid, so that it will get freed when it's out of loop */
	if(!client) {
		return;
	}
	log_info("marking client %hu fd %d as invalid\n", client->id, client->fd);
	client->invalid = 1;
	if(client->id != UNIDENTIFIED_CLIENT) {
		/* also if it is assigned in the global array, then remove it from there also */
		if(client_array[client->id] == client) {
			client_array[client->id] = NULL;
		}
	}
	if(invalid_client_list == NULL) {
		next = NULL;
	} else {
		next = invalid_client_list;
	}
	invalid_client_list = malloc(sizeof(struct _client_list));
	invalid_client_list->client = client;
	invalid_client_list->next = next;
}

static void free_client(struct _client *client)
{
	if(!client)
		return;

	log_info("free client id %hu fd %d\n", client->id, client->fd);

	if(client->fd >= 0) {
		/* this will remove it from epoll */
		close(client->fd);
	}
	free(client->read_buffer.buffer);
	free(client->write_buffer.buffer);
	/* just an assertion to ensure that we are freeing the right thing */
	assert(client_array[client->id] != client);
	free(client);
}

void free_invalid_clients()
{
	struct _client_list *node, *next=NULL;

	for(node=invalid_client_list; node != NULL; node=next) {
		free_client(node->client);
		next = node->next;
		free(node);
	}
	invalid_client_list = NULL;
}

void
add_new_client_to_array(struct _client *client, uint16_t id)
{
	log_info(" add id %hu fd %d\n", id, client->fd);
	mark_client_as_invalid(client_array[id]);
	client_array[id] = client;
	client->id = id;
	/* here we have to chance to store the unset data from previous client */
}

struct _client *find_client_by_id(uint16_t id)
{
	return client_array[id];
}
