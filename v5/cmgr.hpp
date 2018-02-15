#include <iostream>
#include <list>
#include "client.hpp"

using namespace std;

class ClientMgr {
private:
	Client *client_array[MAX_CLIENTS];
	/*
	 * The reason that we mark the clients as invalid and not free them straightaway is that
	 * there might be an event of stale connection lying in the epoll loop, and if we free
	 * the client right away while handling another connection, we might end up in a segfault.
	 * Therefore, over here we mark the client and free the entire list at the end of loop.
	 */
	list<Client*> invalid_client_list;

public:
	ClientMgr():client_array() {}
	void mark_client_as_invalid(Client *client)
	{
		/* mark the client as invalid, so that it will get freed when it's out of loop */
		if(!client) {
			return;
		}
		cout << "marking client " << client->id << "  fd " << client->fd << " as invalid\n";
		client->invalid = true;
		if(client->id != UNIDENTIFIED_CLIENT) {
			/* also if it is assigned in the global array, then remove it from there also */
			if(client_array[client->id] == client) {
				client_array[client->id] = NULL;
			}
		}
		invalid_client_list.push_front(client);
		
	}

	void free_invalid_clients()
	{
		for (Client * c : invalid_client_list) {
			delete c;
		}
		invalid_client_list.clear();
	}

	void add_new_client_to_array(Client *client, uint16_t id)
	{
		cout << " add id " << id << " fd " << client->fd;
		mark_client_as_invalid(client_array[id]);
		client_array[id] = client;
		client->id = id;
		/* here we have to chance to store the unset data from previous client */
	}

	Client *find_client_by_id(uint16_t id)
	{
		return client_array[id];
	}
};
