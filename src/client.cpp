#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <unistd.h>
#include <string.h>

#include "io/tcp_client.hpp"

using namespace networking;

//----------------------------------------------------------------------


class MyClient : public TCPClient
{
    int count=10;
    int num=0;
public:
	MyClient(std::string server_host, uint16_t server_port)
        : TCPClient(server_host, server_port)
    {}

	virtual void on_write_ready(uint32_t server_sfd) {
        TCPClient::on_write_ready(server_sfd);


   /*     int16_t bytes = read_from_socket(server_sfd, buffer_.data(), buffer_.size());
        std::cout << "    bytes read ~" << bytes << " bytes" << std::endl;
        for (int i = 0; i<bytes; i++) 
            std::cout << buffer_.data()[i];*/

        std::cout << __func__ << " Id=" << server_sfd << std::endl;  
	}

	virtual int32_t on_get_standby() {
    	//std::cout << __func__ << std::endl;
        return 10;		
	}

    virtual void on_disconnect(uint32_t server_sfd)
    {
       TCPClient::on_disconnect(server_sfd);
       std::cout << __func__ << std::endl; 
    }

    virtual void on_loop() 
    {
        if(count > 0)
        {
            char message[100];
            sprintf(message, "Hello from client %dx2=%d [%d]\n", count, count*2, num++);

            bool try_again=true;
            send_to_peer((uint8_t*)message, strlen(message), try_again); 
            count--;
        }
    	//std::cout << __func__ << std::endl;
    }

    virtual void handle_data_received(const uint8_t* data, uint16_t data_len)
    {
        std::cout << __func__ << std::endl;
        for (int i = 0; i<data_len; i++) 
            std::cout << data[i];
    }

};



int main()
{
    MyClient client("127.0.0.1", 9000);
    client.run(); 

    return 0;
}
