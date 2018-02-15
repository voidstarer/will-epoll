#include "io/tcp_server.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>



namespace networking {


//----------------------------------------------------------------------

void TCPServer::on_data_received(uint32_t sfd)
{
    std::cout << __func__ << "  sfd=" << sfd << std::endl;

    // This is a little different to the client because it connects if they are not the same
    /* check if it is a listening fd or not */
    if (sfd != socket_fd()) {
        int bytes = read_from_socket(sfd, buffer_.data(), buffer_.size());
        
        std::cout << "~" << __func__ << " " << bytes << " bytes" << std::endl;

        if (bytes > 0) {
	    /* VP: successfully read */
            handle_data_received(sfd, buffer_.data(), bytes);
	} else if (bytes == 0) {
	    /* VP: connection is blocked, we will wait for next turn */
            std::cout << __func__ << "Blocked: Client=" << sfd << std::endl;
        } else {
	    /* fatal error or connection close */
            on_disconnect(sfd);
        }
    } else {
	/* new connection arrived */
        std::cout << "\t  Accept Connection " << std::endl;
        accept_connection();
    }
}


//----------------------------------------------------------------------

void TCPServer::accept_connection()
{
    std::cout << __func__ << " " << std::endl;

    struct sockaddr_in remote_addr;
    uint32_t socklen = sizeof(remote_addr);

    int32_t fd = accept(socket_fd(), reinterpret_cast<struct sockaddr*>(&remote_addr), &socklen);
 
    if (fd > 0 && 
        add_event_socket(fd) && 
        set_socket_nonblocking(fd)) 
    {
        std::cout << "accepted new connection for socket " << fd << std::endl;
        on_connect(fd);
        return;
    }

    close(fd);
}

//----------------------------------------------------------------------

void TCPServer::bind_port()
{
    std::cout << __func__ << " " << std::endl;

    struct sockaddr_in server_addr;
    uint32_t socklen = sizeof(struct sockaddr_in);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_server_port);
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    int32_t sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        throw std::runtime_error("Unable to create TCP socket.");
    }

    int32_t enable = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    if (!set_socket_nonblocking(sfd)) {
        throw std::runtime_error("Unable to set socket to nonblocking mode.");
    }

    if (bind(sfd, reinterpret_cast<const struct sockaddr*>(&server_addr), socklen) < 0) {
        throw std::runtime_error("Unable to bind TCP port for listening.");
    }

    listen(sfd, SOMAXCONN);

    set_socket_fd(sfd);
}



} // namespace networking
