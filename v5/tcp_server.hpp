#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include <iostream>
#include <sys/epoll.h>
#include <ctime>
#include "packet.hpp"
#include "client.hpp"
#include "cmgr.hpp"

#define MAX_EVENTS 100
#define EPOLL_LOOP_TIMEOUT 40 /* seconds */

class TCPServer
{ 
private:
	int server_port;
	int loop_timeout;
	int listen_fd;
	int epoll_fd;
	int set_socket_nonblocking(int sock);
	int bind_port();
	void handle_event(const struct epoll_event *eptr);
	void handle_write_event(void *ptr);
	void handle_read_event(void *ptr);
	int read_packets(Client *client);
	int process_packets(Client *client);
	int validate_packet(Client *client, Packet *packet);
	time_t get_monotonic_time();
	void ignore_sigpipe();
	void accept_connection();
	int unregister_read(Client *client);
	int unregister_write(Client *client);
	int register_write(Client *client);
	int register_event(int fd, int op, uint32_t events, void *data_ptr);
	char * get_events_string(char *buffer, int bufsize, uint32_t events);
	ClientMgr cmgr;
public:
	TCPServer(uint16_t p, int t) : server_port(p), loop_timeout(t), listen_fd(-1), epoll_fd(-1)
	{ }
	virtual void on_connect(Client *client) = 0;
	virtual void on_disconnect(Client *client) = 0;
	virtual void on_data_received(Client *client, Packet *packet) = 0;
	virtual void on_error(Client *client, int32_t error_code) = 0;
	virtual void house_keeping() = 0;
	virtual bool should_i_quit() = 0;

	int do_poll();
	int initialize();
	bool send_data_to_client(Packet *);
};

#endif
