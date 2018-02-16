#include "cmgr.hpp"
#include "log.hpp"

void ClientMgr::~ClientMgr()
{
	int i;
	Client *client;
	for(i=0; i<MAX_CLIENTS; i++) {
		client = client_array[i];
		if(client) {
			delete client;
		}
	}
	free_invalid_clients();
}

void ClientMgr::mark_client_as_invalid(Client *client)
{
	/* mark the client as invalid, so that it will get freed when it's out of loop */
	if(!client) {
		return;
	}
	log_info("marking client %hu fd  %d as invalid", client->id, client->fd);
	client->invalid = true;
	if(client->id != UNIDENTIFIED_CLIENT) {
		/* also if it is assigned in the global array, then remove it from there also */
		if(client_array[client->id] == client) {
			client_array[client->id] = NULL;
		}
	}
	invalid_client_list.push_front(client);
	
}

void ClientMgr::free_invalid_clients()
{
	for (Client * c : invalid_client_list) {
		delete c;
	}
	invalid_client_list.clear();
}

void ClientMgr::add_new_client_to_array(Client *client, uint16_t id)
{
	cout << " add id " << id << " fd " << client->fd;
	mark_client_as_invalid(client_array[id]);
	client_array[id] = client;
	client->id = id;
	/* here we have to chance to store the unset data from previous client */
}

Client* ClientMgr::find_client_by_id(uint16_t id)
{
	return client_array[id];
}

