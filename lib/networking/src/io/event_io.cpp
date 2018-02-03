#include "io/event_io.hpp"
#include <iostream>
#include <sys/time.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>


namespace networking {



// Initialises the class creating a socket 
EventIO::EventIO(uint16_t max_events)
    : _max_events(max_events)
{
//    std::cout << __func__ << std::endl;

    _events.resize(max_events);

#ifdef OS_LINUX
    // creates a new epoll instance. 
    // Since Linux 2.6.8 the size parameter is ignored but must be greater than zero
    int32_t fd = epoll_create(_max_events);
#else
    int32_t fd = kqueue();
#endif

    if (fd == -1) {
        throw std::runtime_error("Unable to create epoll socket.");
    }

    _event_sockfd = fd;  // Note fd is signed, and event_sockfd is unsigned
    std::cout << "\tSocket Created #" << _event_sockfd << std::endl;
}

//----------------------------------------------------------------------

EventIO::~EventIO()
{
//    std::cout << __func__ << " " << std::endl;
    close(_event_sockfd);
}


//----------------------------------------------------------------------

// fd is the socket we want to listen to
bool EventIO::add_event_socket(uint32_t peer_sfd)  
{
//    std::cout << __func__ << " " << peer_sfd << std::endl;

#ifdef OS_LINUX
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = peer_sfd;        // Listen for events from this socket
    ev.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    // Register the target file descriptor fd on the epoll instance
    // referred to by the file descriptor epfd and associate the
    // event event with the internal file linked to fd. 
    if (epoll_ctl(_event_sockfd, EPOLL_CTL_ADD, peer_sfd, &ev) == 0) {
        return true;
    }
#else
    EV_SET(&ev_, peer_sfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    if (kevent(_event_sockfd, &ev_, 1, nullptr, 0, nullptr) != -1) {
        return true;
    }
#endif

    std::cout << ">>>>>>>>>>>>>> UNABLE to ADD SOCKET" << std::endl;

    return false;
}

//----------------------------------------------------------------------

bool EventIO::delete_event_socket(uint32_t peer_sfd)
{
 //   std::cout << __func__ << peer_sfd << std::endl;

#ifdef OS_LINUX
    if (epoll_ctl(_event_sockfd, EPOLL_CTL_DEL, peer_sfd, &ev_) == 0) {
        return true;
    }
#else
    EV_SET(&ev_, peer_sfd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    if (kevent(_event_sockfd, &ev_, 1, nullptr, 0, nullptr) != -1) {
        return true;
    }
#endif

    std::cout << ">>>>>>>>>>>>>> UNABLE to DEL SOCKET" << std::endl;

    return false;
}


//----------------------------------------------------------------------

// Add the ability to write to the socket
// I think you must use add_socket (above) first.
bool EventIO::set_out_event(uint32_t peer_sfd)
{
 //   std::cout << __func__ << " " << peer_sfd << std::endl;

#ifdef OS_LINUX
    struct epoll_event ev;
    ev.data.fd = peer_sfd;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    if (epoll_ctl(_event_sockfd, EPOLL_CTL_MOD, peer_sfd, &ev) == 0) {
        return true;
    }
#else
    EV_SET(&ev_, peer_sfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    if (kevent(_event_sockfd, &ev_, 1, nullptr, 0, nullptr) != -1) {
        return true;
    }
#endif

    std::cout << ">>>>>>>>>>>>>> UNABLE to MOD SOCKET; errno " << errno << std::endl;

    return false;
}

//----------------------------------------------------------------------

// Removes the ability to write to a socket
bool EventIO::unset_out_event(uint32_t peer_sfd)
{
    std::cout << __func__ << " " << peer_sfd << std::endl;

#ifdef OS_LINUX
    struct epoll_event ev;
    ev.data.fd = peer_sfd;
    ev.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    if (epoll_ctl(_event_sockfd, EPOLL_CTL_MOD, peer_sfd, &ev) == 0) {
        return true;
    }
#else
    EV_SET(&ev_, peer_sfd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    if (kevent(_event_sockfd, &ev_, 1, nullptr, 0, nullptr) != -1) {
        return true;
    }
#endif

    std::cout << ">>>>>>>>>>>>>> UNABLE to MOD SOCKET; errno " << errno << std::endl;

    return false;
}

//----------------------------------------------------------------------

void EventIO::handle_event(uint32_t peer_sfd, int32_t ev)
{
    std::cout << __func__ << " " << peer_sfd << std::endl;

#ifdef OS_LINUX
    if (ev & EPOLLIN)    {std::cout << "EPOLLIN ";}
    if (ev & EPOLLPRI)   {std::cout << "EPOLLPRI ";}
    if (ev & EPOLLOUT)   {std::cout << "EPOLLOUT ";}
    if (ev & EPOLLERR)   {std::cout << "EPOLLERR ";}
    if (ev & EPOLLHUP)   {std::cout << "EPOLLHUP ";}
    if (ev & EPOLLRDHUP) {std::cout << "EPOLLRDHUP ";}
    std::cout << std::endl;

    if (ev & (EPOLLIN|EPOLLERR|EPOLLHUP|EPOLLRDHUP)) {
	std::cout << __func__ << " " << peer_sfd << std::endl;
        on_data_received(peer_sfd);
    } else if (ev & (EPOLLOUT|EPOLLHUP|EPOLLERR)) {
        on_write_ready(peer_sfd);
    } else if ((ev & EPOLLERR) || (ev & EPOLLHUP) || (ev & EPOLLRDHUP)) {
        on_disconnect(peer_sfd);
    } else {
        // unknown event, do nothing
        std::cout << "unknown event" << std::endl;
    }
#else

/*
EVFILT_READ   - tells you when a descriptor has data to read
EVFILT_WRITE  - tells you when it is possible to write to a descriptor
EVFILT_AIO    - not supported under OpenBSD
EVFILT_VNODE  - tells you when things happen to files (written, renamed, deleted, etc …)
EVFILT_PROC   - tells you when things happen to processes (called fork(), exit(), etc …)
EVFILT_SIGNAL - tells you when you’ve received a specified signal
*/
    switch(ev)
    {
        case EVFILT_READ:
            on_data_received(peer_sfd);
            break;
        case EVFILT_WRITE:             
            on_write_ready(peer_sfd);
            break;
        case EV_EOF: 
            on_disconnect(peer_sfd);
            break;
        default:
            std::cout << "unknown event" << std::endl;
            break;
    }
#endif
}



//----------------------------------------------------------------------

void EventIO::loop()
{
//    std::cout << __func__ << std::endl;

    int32_t nev = 0;

    while (_alive) {
        try {
            int32_t timeout = on_get_standby();

        //    std::cout << "wait for event..." << std::endl;

#ifdef OS_LINUX
            nev = epoll_wait(_event_sockfd, _events.data(), _max_events, timeout);
#else
            struct timespec* timeout_ptr = nullptr;

            if (timeout >= 0) {
                static struct timespec ts;                              // WARNING: This isn't thread safe

                uint32_t seconds = timeout / 1000;
                uint32_t milliseconds = timeout % 1000;
                uint64_t nanoseconds = milliseconds * 1000000;

                ts.tv_sec = seconds;
                ts.tv_nsec = nanoseconds;
                timeout_ptr = &ts;
            }

           
            // Register events of interest and determine if any events have ocured
            /*  int kevent(kq, changelist, nchanges, eventlist, nevents, timeout)
                where
                  - **kq** is an identifier
                  - **changelist** and **nchanges** describe the changes made to the events, or NULL and 0, respectively if no changes are made
                  - filtered data is returned into **eventlist** which holds **nevents**
                returns the number of events or zero if timeout has occured.
            */
            nev = kevent(_event_sockfd, nullptr, 0, _events.data(), _max_events, timeout_ptr);
#endif
//            std::cout << "nev " << nev << "; errno " << errno << std::endl;

            // Loop through each of the network events
            for (uint16_t n = 0; n < nev; ++n) {

#ifdef OS_LINUX
                uint32_t peer_sfd = _events[n].data.fd;       // I think this is our peers socketfd (or unique ID)
                uint32_t ev = _events[n].events;
#else
                uint32_t peer_sfd = _events[n].ident;
                uint32_t ev = _events[n].filter;
#endif

                handle_event(peer_sfd, ev);
            }

	    /* VP: This effectively means that it can handle only
	       one client, because on_loop uses the fd which was
	       passed in handle_data_received */
	    /*
            if (!nev) {
                on_loop();
            } */
        } catch (...) {
        }
    }
}


} // namespace networking
