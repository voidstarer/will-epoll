#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include <iostream>
#include "packet.hpp"
#include "client.hpp"


class TCPServer
{ 
	int server_port;
	int listen_fd;
public:
	TCPServer(uint16_t server_port)
	{
		server_port = server_port;
	}
	virtual void on_connect(Client *client) = 0;
	virtual void on_disconnect(Client *client) = 0;
	virtual void on_data_received(Client *client, Packet *packet) = 0;
	virtual void on_error(Client *client, int32_t error_code) = 0;
	virtual void house_keeping() = 0;

	void send_data_to_client(Packet *);
	int do_poll();
};

#endif
