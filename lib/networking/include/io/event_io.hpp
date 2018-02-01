// Good references
//  - http://eradman.com/posts/kqueue-tcp.html


#ifndef __EVENT_IO_HPP__
#define __EVENT_IO_HPP__

#if defined(__linux__)
    #define OS_LINUX
#else
    #define OS_BSD
#endif

#include <functional>
#include <vector>

#if defined(OS_LINUX)
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif

#include <iostream>


namespace networking {


/*

-- Socketfd --
Each running process has a file descriptor table which
contains pointers to all open i/o streams.  When a process
starts, three entries are created in the first three cells of
the table.  Entry 0 points to standard input, entry 1 points
to standard output, and entry 2 points to standard error.
Whenever a file is opened, a new entry is created in this
table, usually in the first available empty slot.

The socket system call returns an entry into this table; i.e.
a small integer.  This value is used for other calls which
use this socket.  The accept system call returns another
entry into this table.  The value returned by accept is used
for reading and writing to that connection.
*/

class EventIO
{
private:
    uint16_t _max_events;     // Maximum number of events that can be handled
 
    // An array of events
#if defined(OS_LINUX)
    struct epoll_event ev_;
    std::vector<struct epoll_event> _events;  
#else
    struct kevent ev_;
    std::vector<struct kevent> _events;
#endif

    uint32_t _event_sockfd;         // Socket descriptor 

protected:
    bool _alive = true;             // Used to signify when the control loop should stop.

public:  // I suspect this can be protected

    // Initialises the class creating a socket 
    EventIO(uint16_t max_events = 1024);
    ~EventIO();

public:
    // --- Change the number of events that can be handled
    void set_max_events(uint16_t max_events) {
        std::cout << __func__ << " " << std::endl;

        _max_events = max_events;
        _events.resize(max_events);
    }

    void loop();

    // --- Add or Delete sockets
    bool add_event_socket(uint32_t peer_sfd);      // Creates socket for reading
    bool delete_event_socket(uint32_t peer_sfd);   // Shuts down the socket

    bool set_out_event(uint32_t peer_sfd);     // Add the ability to write to a socket (I think !)
    bool unset_out_event(uint32_t peer_sfd);  // Removes this ability

protected:
    uint32_t event_socket() { return _event_sockfd; }

    void handle_event(uint32_t peer_sfd, int32_t ev);

    // --- Event handlers. Pure virtual funnction
    virtual void on_connect(uint32_t peer_sfd) = 0;
    virtual void on_data_received(uint32_t peer_sfd) = 0;
    virtual void on_write_ready(uint32_t peer_sfd) = 0;
    virtual int32_t on_get_standby() = 0;
    virtual void on_loop() = 0;

    // Response when we loose a connection
    virtual void on_disconnect(uint32_t peer_sfd)
    {
        delete_event_socket(peer_sfd);
    }

};


} // namespace networking


#endif // __EVENT_IO_HPP__
