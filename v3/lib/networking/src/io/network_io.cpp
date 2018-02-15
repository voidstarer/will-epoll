#include "io/network_io.hpp"

#include <iostream>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>



namespace networking {

//----------------------------------------------------------------------

bool NetworkIO::set_socket_keepalive(uint32_t sockfd)
{
    std::cout << __func__ << " " << std::endl;

    int32_t enable = 1;
    int32_t keep_interval = 5;

    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) != 0) 
        return false;


#ifdef OS_LINUX
    int32_t keep_cnt = 3;
    int32_t keep_idle = 10;

    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, &keep_cnt, sizeof(keep_cnt)) != 0) 
        return false;


    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle)) != 0) 
        return false;
 
    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval)) != 0) 
        return false;
  
#else
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPALIVE, &keep_interval, sizeof(keep_interval)) != 0) 
        return false;
#endif


    return true;
}

//----------------------------------------------------------------------

bool NetworkIO::set_socket_nonblocking(uint32_t sockfd)
{
    std::cout << __func__ << " " << std::endl;

    int32_t flags = 0;
    flags = fcntl(sockfd, F_GETFD, 0);
    flags |= O_NONBLOCK;

//    if (fcntl(sockfd, F_SETFL, flags) < 0) 
//        return false;
    
    return (fcntl(sockfd, F_SETFL, flags) >= 0); 
}

//----------------------------------------------------------------------

/*
 * read_from_socket: Return values
 *     Returns number of bytes that are read from socket during successful read.
 *     0 - if the socket is blocked and nothing is read 
 *     -1 - if connection is closed
 *     -2 - in case of unknown errors
 */
int32_t NetworkIO::read_from_socket(uint32_t sockfd, uint8_t* data_ptr, uint16_t max_len)
{
    //    std::cout << __func__ << " " << std::endl;
    for(;;) {
	int32_t rv = read(sockfd, data_ptr, max_len);

	if (rv > 0) {
	    return rv;
	} else if (rv == 0) {
	    /* VP: connection closed by opposite end */
	    /* VP: do the cleanup */
	    return -1;
	} else {
	    /* VP: check if it has returned due to a signal
	       or has been blocked */
	    if(errno == EINTR) {
		/* VP: loop again */
	    } else if(errno == EAGAIN || errno == EWOULDBLOCK) {
		/* VP: socket is blocked,
		   let's wait for next set of data */
		return 0;
	    } else {
		/* VP: Fatal error, we should not continue */
		return -2;
	    }
	}
    }
    return 0;
}


//----------------------------------------------------------------------
// Returns
// - no of bytes written to an fd, sets try_again=true if fd is blocked
// - -1 in case of an error
int32_t NetworkIO::write_to_socket(uint32_t sockfd, const uint8_t* data_ptr, uint16_t data_len, bool& try_again)
{
//    std::cout << __func__ << " " << std::endl;

    uint32_t left = data_len;

#ifdef __APPLE__
        int32_t flag = SO_NOSIGPIPE;
#else
        int32_t flag = MSG_NOSIGNAL;
#endif


    while (left>0) 
    {
        int32_t rv = send(sockfd, data_ptr, left, flag);

        if (rv == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                try_again = true;
                return data_len - left;
            }

            return rv;
        }

        data_ptr += rv;   // advance pointer to next set of unsent data
        left -= rv;       // keep a tabs on what is left to send
    }

    return data_len - left;
}


int32_t NetworkIO::send_to_peer(uint32_t sockfd, const uint8_t* data, uint16_t data_len, bool& try_again)
{
    std::cout << __func__ << " " << std::endl;

    int32_t bytes = write_to_socket(sockfd, data, data_len, try_again);

    if (bytes == -1) {
        on_disconnect(sockfd);
    } else {
	
    }

    return bytes;
}



} // namespace networking
