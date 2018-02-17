#include <iostream>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include "unistd.h"

#include "client.hpp"
#include "packet.hpp"
#include "tcp_server.hpp"
#include "log.hpp"

using namespace std;

class MyServer : public TCPServer
{
private:
	bool quit;

public:
	MyServer(int server_port, int timeout_in_sec)
		: TCPServer(server_port, timeout_in_sec)
	{}

	void ask_to_quit()
	{
		quit = true;
	}

	bool should_i_quit()
	{
		return quit;
	}

	void on_connect(Client *client)
	{
		cout << __func__ << ": Client connected from ip " << client->ip << " and port " << client->port <<" with fd " << client->fd << endl;
	}

	void on_auth(Client *client)
	{
		cout << __func__ << ": Client " << client->id << " fd " << client->fd << " authenticated\n";
	}

	void on_disconnect(Client *client)
	{
		cout << __func__ << ": id " << client->id << " Disconnected\n";
	}

	void on_data_received(Client *client, Packet *packet)
	{

		switch(packet->type) {
			case PKT_HELLO:
				/* this is a hello packet sent by the client to identify itself */
				cout << __func__ << ": id: " << client->id << " sent HELLO\n";
				break;
			case PKT_DATA:
				/* use this type to send data to another client */
				cout << __func__ << ": src: " << client->id << " => dst " << packet->dst_id << ":: " << packet->to_string() << "\n";
				send_data_to_client(packet);
				break;
			default:
				/* client sent an invalid packet */
				cerr << __func__ << ": client " << client->id << " sent an invalid packet\n";
				break;
		} 
	}

	void on_error(Client *client, int32_t error_code)
	{
		cout << __func__ << ": Error on id " << client->id << ": " << strerror(error_code) << "\n";
	}

	void house_keeping()
	{
		cout << __func__ << ": houseKeeping called\n";
	}

	int run()
	{
		cout << __func__ << ": Initialize\n";
		if(initialize() == false) {
			cerr << __func__ << ": Failed to initialize server\n";
			return(-1);
		}
		cout << __func__ << ": Polling\n";
		do_poll();
		cout << __func__ << ": Exit\n";
		return(0);
	}

};

MyServer *server = NULL;

static void signal_handler(int signo)
{
	
	server->ask_to_quit();
}

static void usage(const char *name)
{
	cerr << "usage: " << name << " [ -p <port>][ -l 0|1|2 ]\nDefault Port=9000.\nLoglevel: 0 - Error, 1 - Info, 2 - Debug\n";
	exit(1);
}

int main(int argc, char **argv)
{
	int ret;
	int optch;
	int server_port=9000;
	int timeout = 40; /* seconds */
	int l=0;

	while ((optch = getopt(argc, argv, "p:l:h")) != EOF) {
		switch(optch) {
			case 'p':
				server_port = atoi(optarg);
				break;
			case 'l':
				l = atoi(optarg);
				break;
			case 'h':
			default:
				usage(argv[0]);
		}
	}

	set_log_level(l);
	server = new MyServer(server_port, timeout);
	signal(SIGINT, signal_handler);
	ret = server->run();
	return ret;
}
