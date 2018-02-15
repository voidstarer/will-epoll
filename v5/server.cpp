#include <iostream>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include "unistd.h"

#include "client.hpp"
#include "packet.hpp"
#include "tcp_server.hpp"

using namespace std;

class MyServer : public TCPServer
{
private:
	bool quit;

public:
	MyServer(uint16_t server_port=9000)
		: TCPServer(server_port)
	{}

	void set_quit()
	{
		quit = true;
	}

	void on_connect(Client *client)
	{
		cout << "Child on_connect called\n";
		//cout << "Client connected from ip " << client->ip << "and port " << client->port <<" with fd " << client->fd << endl;
	}

	void on_disconnect(Client *client)
	{
		cout << "id " << client->id << " Disconnected\n";
	}

	void on_data_received(Client *client, Packet *packet)
	{

		switch(packet->type) {
			case PKT_HELLO:
				/* this is a hello packet sent by the client to identify itself */
				cout << "id: " << client->id << " sent HELLO\n";
				break;
			case PKT_DATA:
				/* use this type to send data to another client */
				cout << "src: " << client->id << " => dst " << packet->dst_id << ":: " << packet->to_string();
				send_data_to_client(packet);
				break;
			default:
				/* client sent an invalid packet */
				cerr << "client " << client->id << " sent an invalid packet";
				break;
		} 
	}

	void on_error(Client *client, int32_t error_code)
	{
		cout << "Error on id " << client->id << ": " << strerror(error_code) << "\n";
	}

	void house_keeping()
	{
		cout << "houseKeeping called\n";
	}

	void run()
	{
		cout << "Running\n";
		while(!quit) {
			cout << "Calling parent do_poll\n";
			do_poll();
			sleep(1);
		}
		cout << "exit\n";
	}

};

MyServer *server = NULL;

static void signal_handler(int signo)
{
	
	server->set_quit();
}

static void usage(const char *name)
{
	cerr << "usage: " << name << " [ -p <port>][ -l 0|1|2 ]\nDefault Port=2222.\nLoglevel: 0 - Error, 1 - Info, 2 - Debug\n";
	exit(1);
}

int main(int argc, char **argv)
{
	int optch;
	int server_port=9000;
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

	server = new MyServer(server_port);
	signal(SIGINT, signal_handler);
	server->run();
	return(0);
}
