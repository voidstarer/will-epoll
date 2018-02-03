#ifndef __TCP_CLIENT_HPP__
#define __TCP_CLIENT_HPP__

#include "event_io.hpp"
#include "network_io.hpp"

#include <memory>
#include <string>
#include <array>

namespace networking {


class TCPClient: public NetworkIO
{
protected:
    bool _connected = false;
    std::string _server_host;

    std::array<int32_t, 2> internal_io_; // unix sockets for signalling to Event IO system, 0th socket for write 1st for read
   
public:
    // Constructor with parameters for connecting to the server
    TCPClient(std::string server_host, uint16_t server_port)
        : NetworkIO(server_port), _server_host(server_host)
    {}

    bool is_connected() const { return _connected; }


    // Run the client
    void run() {
        std::cout << __func__ << std::endl;

        initialize();

        make_connection();
        loop();
    }

    int32_t send_to_peer(const uint8_t* data, uint16_t data_len, bool& try_again) {
        return NetworkIO::send_to_peer(socket_fd(), data, data_len, try_again );
    }

   
    void wakeup_io();



protected:
    void initialize();
    void make_connection();

    virtual void on_connect(uint32_t server_sfd) {
        std::cout << __func__ << " Peer=" << server_sfd << std::endl;

        _connected = true;
        unset_out_event(server_sfd);
    }

    virtual void on_disconnect(uint32_t server_sfd)
    {
       _connected = false;
       _alive = false;
       NetworkIO::on_disconnect(server_sfd);
    }

    virtual void on_data_received(uint32_t server_sfd);

    virtual void on_write_ready(uint32_t server_sfd) {
         if (!_connected) {
            on_connect(server_sfd);
        }
    }


    virtual void handle_data_received(const uint8_t* data, uint16_t data_len) = 0;
};




} // namespace networking


#endif // __TCP_CLIENT_HPP__
