#include <iostream>
#include <string.h>

#include "io/tcp_server.hpp"

using namespace networking;

//----------------------------------------------------------------------

class MyServer : public TCPServer
{
	int count =0;
	uint32_t _client= 0;
	int num = 0;

public:
	MyServer(uint16_t server_port=9000)
		: TCPServer(server_port)
	{}

	virtual void on_connect(uint32_t client_sfd)
	{
		std::cout << __func__ << " Client=" << client_sfd << std::endl;
	}

	virtual void on_disconnect(uint32_t client_sfd)
	{
		std::cout << __func__ << " Client=" << client_sfd << " Closing socket" << std::endl; 
		/* VP: do the cleanup of FD, remove it from epoll and free any data associated with it  */
		close(client_sfd);
	}

	virtual void on_write_ready(uint32_t client_sfd) {
		std::cout << __func__ << " Client=" << client_sfd << std::endl;	
	}

	/* returns timeout for epoll */
	virtual int32_t on_get_standby() {
		//	std::cout << __func__ << std::endl;
		return 10;	
	}

	virtual void on_loop() {
		//	std::cout << __func__ << std::endl;
	}

	int send_reply_to_client(int client_sfd) {
		char message[100];
		sprintf(message, "Hello from server %dx2=%d [%d]\n", count, count*2, num++);

		bool try_again=true;
		return send_to_peer(_client, (uint8_t*)message, strlen(message), try_again); 
	}

	virtual void handle_data_received(uint32_t client_sfd, const uint8_t* data, uint16_t data_len)
	{
		count++;
		_client = client_sfd;
		std::cout << __func__ << " count = " << count << " data_len=" << data_len << std::endl;
		for (int i = 0; i<data_len; i++) 
			std::cout << data[i];
		std::cout << __func__ << std::endl;
		send_reply_to_client(client_sfd);
	}
};


int main()
{
	MyServer server(9000);
	server.run();
	return 0;
}
