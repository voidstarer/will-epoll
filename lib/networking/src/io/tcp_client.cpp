#include "io/tcp_client.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <iostream>




namespace networking {


//----------------------------------------------------------------------

void TCPClient::initialize()
{
    std::cout << __func__ << std::endl;

    socketpair(AF_UNIX, SOCK_STREAM, 0, internal_io_.data());
    if (!set_socket_nonblocking(internal_io_.at(0)) || set_socket_nonblocking(internal_io_.at(1))) {
        std::runtime_error("Unable to set internal io sockets to non blocking mode.");
    }

    if (!add_event_socket(internal_io_.at(1))) { // 0th socket for write, 1st for read
        std::runtime_error("Unable to add internal io sockets to event IO.");
    } 
}

//----------------------------------------------------------------------

void TCPClient::make_connection()
{
    std::cout << __func__ << std::endl;

    // VINOD - sockfd should be initialized to -1
    int32_t sockfd = 0;
    uint16_t attempt = 0;

    while (true) {
        if (attempt++) {
	    // VINOD - Not the correct way of checking, put a epoll/select/poll around to verify if the socket has successfully connected or not
            sleep(1); // if it is not first attempt - waiting to avoid flooding
        }

        if (sockfd) { // closing local socket which may be opened
            close(sockfd);
            sockfd = 0;
        }

        if (socket_fd()) { // closing object socket which may be opened too
            close_socket_fd();
        }

        struct sockaddr_in remote;
        socklen_t socklen = sizeof(struct sockaddr_in);

        remote.sin_family = AF_INET;
        remote.sin_port = htons(_server_port);
        remote.sin_addr.s_addr = inet_addr(_server_host.c_str());

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0 
            || !set_socket_nonblocking(sockfd)
           // || !set_socket_keepalive(sockfd) 
            )
        {
	    // VINOD - Socket creation will fail only when the system is super over loaded. Why would you want to 
	    // loop when the socket-non-blocking cannot be set ?
            continue;
        }


        int32_t rv = connect(sockfd, reinterpret_cast<const struct sockaddr*>(&remote), socklen);
        if (rv < 0) {
            if (errno != EINPROGRESS) {
                continue;
            }
        }

	// VINOD - add_event_socket : adds to epoll, and after that it called set_out_event to register for write event. Why two system calls ?
        if (!add_event_socket(sockfd) || !set_out_event(sockfd) ) {
            delete_event_socket(sockfd);
            continue;
        }

        set_socket_fd(sockfd);

        break;
    }
}



//----------------------------------------------------------------------

void TCPClient::on_data_received(uint32_t sfd)
{
    std::cout << __func__ << " Peer=" << sfd << std::endl;

    // If the data is destined for this socket
    if (sfd == socket_fd()) {
        int16_t bytes = read_from_socket(sfd, buffer_.data(), buffer_.size());
        std::cout << "    bytes read ~" << bytes << " bytes" << std::endl;

        if (bytes > 0) 
            handle_data_received(buffer_.data(), bytes);
        else {
            on_disconnect(sfd);
        }
    } else {
        // ignoring io signalling
        // std::cout << "signalled" << std::endl;
        on_write_ready(sfd);
    }
}

//----------------------------------------------------------------------


//----------------------------------------------------------------------

void TCPClient::wakeup_io()
{
    std::cout << __func__ << " " << std::endl;

    static const std::string wakeup_message = "wakeup";
    write(internal_io_.at(0), wakeup_message.data(), wakeup_message.size());
}


} // namespace networking
