#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include <iostream>
#include <sys/epoll.h>
#include <ctime>
#include "packet.hpp"
#include "client.hpp"
#include "cmgr.hpp"
#include "event.hpp"

class TCPServer : public Event
{ 
private:
	int server_port;
	int loop_timeout;
	int listen_fd;
	bool bind_port();
	bool register_read(Client *client);
	bool register_write(Client *client);
	bool unregister_write(Client *client);
	bool unregister_read(Client *client);
	void accept_connection();
	void ignore_sigpipe();
	time_t get_monotonic_time();
	int process_packet(Client *client, Packet *P);
	void handle_read_event(void *ptr);
	void handle_write_event(void *ptr);
	void handle_event(const struct epoll_event *eptr);
	ClientMgr cmgr;
public:
	TCPServer(int p, int t) : Event(), server_port(p), loop_timeout(t), listen_fd(-1)
	{ }
	virtual void on_connect(Client *client) = 0;
	virtual void on_auth(Client *client) = 0;
	virtual void on_disconnect(Client *client) = 0;
	virtual void on_data_received(Client *client, Packet *packet) = 0;
	virtual void on_error(Client *client, int32_t error_code) = 0;
	virtual void house_keeping() = 0;
	virtual bool should_i_quit() = 0;

	void do_poll();
	bool initialize();
	bool send_data_to_client(Packet *);
};

#endif
