#ifndef __NETWORK_IO_HPP__
#define __NETWORK_IO_HPP__


#include "event_io.hpp"

#include <iostream>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>


namespace networking {


/*
    Simple class that configures and then reads-to or writes-to a socket.

    This is provides the core functionality used by either a client and server.
*/
class NetworkIO : public EventIO
{
private:
    uint32_t _sockfd = 0;

protected:
    std::array<uint8_t, 102400> buffer_;     // 100 kb input buffer
    uint16_t _server_port;               // Port that the server communicated on, i.e. 9000

    // Constructor
    NetworkIO(uint16_t server_port, uint16_t max_events = 1024)
        : EventIO(max_events), _server_port(server_port)
    {}

    // --- Manage the socket
    uint32_t socket_fd() { return _sockfd; }
    void set_socket_fd(uint32_t socket) { _sockfd=socket; }
    void close_socket_fd() { 
        close(_sockfd);
        _sockfd = 0;
    }
    // ---

    bool set_socket_keepalive(uint32_t sockfd);
    bool set_socket_nonblocking(uint32_t sockfd);

    int32_t read_from_socket(uint32_t sockfd, uint8_t*, uint16_t max_len);
    int32_t write_to_socket(uint32_t sockfd, const uint8_t* data_ptr, uint16_t data_len, bool& try_again);

    int32_t send_to_peer(uint32_t sockfd, const uint8_t* data, uint16_t data_len, bool& retry);
};



} // namespace networking


#endif // __NETWORK_IO_HPP__
