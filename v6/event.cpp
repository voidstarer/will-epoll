#include <cstring>
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

	assert(fd >= 0);
	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.ptr = (void *)data_ptr;

	log_debug(" fd: %d events: %s\n", fd, get_events_string(buffer, sizeof(buffer), events));

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



