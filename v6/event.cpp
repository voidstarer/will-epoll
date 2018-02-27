#include <cstring>
#include <fcntl.h>
#include "event.hpp"
#include "log.hpp"

bool Event::initialize()
{
	/* create a new epoll */
	if ((epoll_fd = epoll_create(MAX_EVENTS)) == -1) { 
		log_err("epoll_create: failed: %s\n", strerror(errno));
		return false;
	}
	return true;
}

bool Event::register_event(int fd, int op, uint32_t events, void *data_ptr)
{
	struct epoll_event ev;
	char buffer[1024];
	const char *opstr;

	assert(fd >= 0);
	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.ptr = (void *)data_ptr;

	if(op == EPOLL_CTL_ADD) {
		opstr = "ADD";
	} else if(op == EPOLL_CTL_MOD) {
		opstr = "MODIFY";
	} else if(op == EPOLL_CTL_DEL) {
		opstr = "DEL";
	} else {
		opstr = "UNKNOWN";
	}

	log_debug(" fd: %d op=%s events: %s\n", fd, opstr,
		get_events_string(buffer, sizeof(buffer), events));

	if (epoll_ctl(epoll_fd, op, fd, &ev) == -1 ) {
		log_err(" epoll_ctl failed: %s\n", strerror(errno));
		return false;
	}
	return true;
}

char * Event::get_events_string(char *buffer, int bufsize, uint32_t events) 
{
	snprintf(buffer, bufsize, "%s%s%s%s%s%s",
		events & EPOLLIN?"EPOLLIN":"", events & EPOLLOUT?" EPOLLOUT":"",
		events & EPOLLHUP?" EPOLLHUP":"", events & EPOLLRDHUP?" EPOLLRDHUP":"",
		events & EPOLLERR?" EPOLLERR":"", events & EPOLLPRI?" EPOLLPRI":"");
	return buffer;
}

/*
 * Set the socket to non blocking 
 */
int Event::set_socket_nonblocking(int sock)
{
	int flags;

	assert(sock >= 0);

	flags = fcntl(sock, F_GETFL, 0);
	return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}
