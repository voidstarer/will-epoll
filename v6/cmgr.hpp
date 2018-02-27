#ifndef _CMGR_HPP_
#define _CMGR_HPP_
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
	ClientMgr():client_array(), invalid_client_list(){}
	~ClientMgr();
	void mark_client_as_invalid(Client *client);
	void free_invalid_clients();
	void manage(Client *client);
	Client *find_client_by_id(uint16_t id);
};
#endif
