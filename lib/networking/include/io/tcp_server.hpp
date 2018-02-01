#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include <array>
#include <functional>
#include <memory>
#include "event_io.hpp"
#include "network_io.hpp"



namespace networking {


class TCPServer: public NetworkIO
{  
public:
    TCPServer(uint16_t server_port)
        : NetworkIO(server_port)
    {}


    void run()
    {
        std::cout << __func__ << std::endl;
        initialize();
        loop();
    }

   
protected:
    virtual void on_data_received(uint32_t client_fd);  

    void accept_connection();
    
    // Bind the port to ??
    void bind_port();

    void initialize()
    {
        std::cout << __func__ << " " << std::endl;

        bind_port();

        if (!add_event_socket(socket_fd())) {
            throw std::runtime_error("Unable to add socket to event IO.");
        }
    }


    virtual void handle_data_received(uint32_t client_fd, const uint8_t* data, uint16_t data_len) = 0;
};




} // namespace networking


#endif // __TCP_SERVER_HPP__
