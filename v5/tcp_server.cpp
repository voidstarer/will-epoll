#include <iostream>
#include "tcp_server.hpp"
#include "cmgr.hpp"


void TCPServer::send_data_to_client(Packet *p)
{
}


int TCPServer::do_poll()
{
	std::cout << "TCPServer do_poll called\n";
	on_connect(NULL);
	return 0;
}

