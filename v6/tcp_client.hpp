#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include <iostream>
#include <sys/epoll.h>
#include <ctime>
#include "packet.hpp"
#include "client.hpp"
#include "cmgr.hpp"
#include "event.hpp"

typedef enum {
	DISCONNECTED,
	CONNECTING,
	CONNECTED
} server_state;

class TCPClient : public Event
{ 
private:
	char server_ip[20];
	int server_port;
	int loop_timeout;
	int server_fd;
	server_state state;
	uint16_t id;
	Client *me;
	bool connect_to_server();
	bool check_connect(uint32_t events, int fd);
	bool register_read(Client *client);
	bool register_write(Client *client);
	bool unregister_write(Client *client);
	bool unregister_read(Client *client);
	void ignore_sigpipe();
	time_t get_monotonic_time();
	int process_packet(Client *client, Packet *P);
	void handle_read_event(void *ptr);
	void handle_write_event(void *ptr);
	void handle_event(const struct epoll_event *eptr);
	void handle_initial_state(const struct epoll_event *eptr);
	void disconnect();
	void send_packet(packet_type type, uint16_t dst_id, const uint8_t *data, uint32_t data_len);
	void send_hello();
public:
	TCPClient(const char *ip, int p, int t, uint16_t id) : Event(), server_port(p), loop_timeout(t), server_fd(-1), state(DISCONNECTED), id(id)
	{
		snprintf(server_ip, sizeof(server_ip), "%s", ip);
	}
	virtual void on_connect(Client *client) = 0;
	virtual void on_auth(Client *client) = 0;
	virtual void on_disconnect(Client *client) = 0;
	virtual void on_data_received(Client *client, Packet *packet) = 0;
	virtual void on_error(Client *client, int32_t error_code) = 0;
	virtual void house_keeping() = 0;
	virtual bool should_i_quit() = 0;

	void do_poll();
	bool initialize();
	bool send_data_to_server(Packet *);
};

#endif
