#ifndef __EVENT_HPP__
#define __EVENT_HPP__

#include <iostream>
#include <sys/epoll.h>
#include <ctime>
#include <assert.h>
#include "packet.hpp"
#include "client.hpp"
#include "cmgr.hpp"

#define MAX_EVENTS 100
#define EPOLL_LOOP_TIMEOUT 40 /* seconds */

class Event
{ 
protected:
	int epoll_fd;
	bool initialize();
	char * get_events_string(char *buffer, int bufsize, uint32_t events);
	bool register_event(int fd, int op, uint32_t events, void *data_ptr);

public:
	Event() : epoll_fd(-1)
	{ }
	virtual void on_connect(Client *client) = 0;
	virtual void on_auth(Client *client) = 0;
	virtual void on_disconnect(Client *client) = 0;
	virtual void on_data_received(Client *client, Packet *packet) = 0;
	virtual void on_error(Client *client, int32_t error_code) = 0;
	virtual void house_keeping() = 0;
	virtual bool should_i_quit() = 0;

};

#endif
