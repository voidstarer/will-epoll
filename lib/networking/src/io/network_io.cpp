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

int32_t NetworkIO::read_from_socket(uint32_t sockfd, uint8_t* data_ptr, uint16_t max_len)
{
//    std::cout << __func__ << " " << std::endl;

    int32_t rv = read(sockfd, data_ptr, max_len);

    if (rv > 0) return rv;

    if (errno != 0) 
        return -1;
        
    return 0;
}


//----------------------------------------------------------------------
// Return the number of bytes read
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

    if (bytes == -1 && !try_again) {
        on_disconnect(sockfd);
    }

    return bytes;
}



} // namespace networking
