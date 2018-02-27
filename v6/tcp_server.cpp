#include <iostream>
#include <cstring>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>


#include "log.hpp"
#include "tcp_server.hpp"
#include "cmgr.hpp"

/*
 * Set the socket to non blocking 
 */
int TCPServer::set_socket_nonblocking(int sock)
{
	int flags;

	assert(sock >= 0);

	flags = fcntl(sock, F_GETFL, 0);
	return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

bool TCPServer::bind_port()
{
	struct sockaddr_in addr;
	int fd;
	const int on = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	addr.sin_addr.s_addr = inet_addr("0.0.0.0");

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		log_err(" Unable to bind listening socket to port %d because of %s\n",
				server_port, strerror(errno));
		close(fd);
		return false;
	}

	if (listen(fd, 20) < 0) {
		log_err("Unable to start listening socket because of %s\n", strerror(errno));
		close(fd);
		return false;
	}
	set_socket_nonblocking(fd);
	listen_fd = fd;
	return true;
}

bool TCPServer::register_read(Client *client)
{
	assert(client != NULL);
	log_info("called for id %d fd %d\n", client->id, client->fd);
	return register_event(client->fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP, client);
}

bool TCPServer::register_write(Client *client)
{
	assert(client != NULL);
	if(client->write_registered == true) {
		return true;
	}
	client->write_registered = true;
	log_info("called for id %d fd %d\n", client->id, client->fd);
	return register_event(client->fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP, client);
}

bool TCPServer::unregister_write(Client *client)
{
	assert(client != NULL);
	if(client->write_registered == false) {
		return true;
	}
	client->write_registered = false;
	log_info("called for id %d fd %d\n", client->id, client->fd);
	return register_event(client->fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP, client);
}

bool TCPServer::unregister_read(Client *client)
{
	struct epoll_event ev;

	assert(client != NULL);
	/* prevent valgrind from shouting */
	memset(&ev, 0, sizeof(ev));

	log_info(" id: %u fd: %d\n", client->id, client->fd);

	/* NOTE: Kernel below 2.6.9 need NON-NULL ev to be passed every time
	 * even though its of no use for delete. Thats y i have to pass it
	 * on here to be safe.
	 */
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->fd, &ev) == -1 ) {
		log_err(" epoll_ctl failed: %s\n", strerror(errno));
		return false;
	}
	return true;
}


void TCPServer::accept_connection()
{
	int newfd;
	socklen_t size;
	struct sockaddr_in cl;
	Client *client;

	size = sizeof(struct sockaddr_in);
	newfd = accept(listen_fd, (struct sockaddr *)&cl, &size);
	if (newfd < 0) {
		log_err("accept() failed: %s\n", strerror(errno));
		return;
	}

	set_socket_nonblocking(newfd);
	client = new Client(newfd);
	inet_ntop(AF_INET, &cl.sin_addr, client->ip, sizeof(client->ip));
	client->port = ntohs(cl.sin_port);

	/* onConnect */
	on_connect(client);
	register_read(client);
	return;
}


void TCPServer::ignore_sigpipe()
{
	struct sigaction ac;
	/* ignore sigpipe, let write get fail */
	memset(&ac, 0, sizeof ac);
	sigaction(SIGPIPE, NULL, &ac);
	ac.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &ac, NULL);
}

time_t TCPServer::get_monotonic_time()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		return 0;
	}
	return ts.tv_sec;
}

void TCPServer::handle_read_event(void *ptr)
{
	Client *client = (Client*)ptr;
	Packet *P;
	int ret;

	assert(client != NULL);
	if(client->invalid) {
		/* this is a stale client */
		log_err(" id: %u fd: %d is invalid\n", client->id, client->fd);
		return;
	}
	log_info("id: %hu fd: %d\n", client->id, client->fd);

	P = client->read_packet(ret);
	if(P) {
		cmgr.manage(client);
		/* here I am assuming that on_data_received has copied the relevant data.
		 * If you want to prevent copy, you can use this packet itself for
		 * sending to destination, in that case, remove the delete here
		 */
		on_data_received(client, P);
		delete P;
	} else if(ret == 0) {
		/* we received a handshake packet,
		   and the packed got consumed internally
		   within the client*/
		on_auth(client);
		cmgr.manage(client);
		log_info(" id: %hu: packet internally consumed\n", client->id);
	} else {
		if(ret == EAGAIN) {
			/* epoll will bring us back */
			log_info(" id: %hu returning back to epoll\n", client->id);
			return;
		} else if(ret == EMSGSIZE) {
			log_err( " id: %hu sent a large message\n", client->id);
			on_error(client, EMSGSIZE);
		} else if(ret == EINVAL) {
			log_err( " id: %hu sent an invalid message\n", client->id);
			on_error(client, EINVAL);
		} else if(ret == ESHUTDOWN) {
			on_disconnect(client);
			log_info(" id: %hu closed the connection\n", client->id);
		} else {
			on_error(client, errno);
			log_err(" id: %hu fd:%d read failed\n", client->id, client->fd);
		}
		cmgr.mark_client_as_invalid(client);
	}
}

bool TCPServer::send_data_to_client(Packet *P)
{
	Client *client;
	client = cmgr.find_client_by_id(P->dst_id);
	if(!client) {
		log_err("client %hu not found\n", P->dst_id);
		return -1;
	}
	client->queue_packet(P);
	return register_write(client);
}

void TCPServer::handle_write_event(void *ptr)
{
	Client *client = (Client*)ptr;
	assert(client != NULL);
	if(client->invalid) {
		/* this is a stale client */
		log_err("id: %u fd: %d is invalid\n", client->id, client->fd);
		return;
	}

	log_info("called for id: %u fd: %d\n", client->id, client->fd);

	if(client->has_write_pending() == false) {
		log_debug("id: %hu nothing more to write, will remove write registration\n", client->id);
		unregister_write(client);
		return;
	}

	if(client->write_data() == false) {
		/* terrible error, free the client */
		on_error(client, errno);
		cmgr.mark_client_as_invalid(client);
	}

	return;
}


void TCPServer::handle_event(const struct epoll_event *eptr)
{
	char buffer[1024];
	uint32_t ev = eptr->events; 

	log_debug("%s\n", get_events_string(buffer, sizeof(buffer), ev));

	if (ev & (EPOLLIN|EPOLLERR|EPOLLHUP|EPOLLRDHUP)) {
		handle_read_event(eptr->data.ptr);
	} else if (ev & (EPOLLOUT|EPOLLHUP|EPOLLERR)) {
		handle_write_event(eptr->data.ptr);
	} else {
		/* we should never reach here */
		assert(NULL && "we should never reach here");
	}
}

bool TCPServer::initialize()
{
	ignore_sigpipe();

	if(Event::initialize() == false) {
		return false;
	}

	if(bind_port() == false) {
		log_err("failed to create listen socket\n");
		return false;
	}

	/* listen_fd is explicitly registered with EPOLLIN only */
	if(register_event(listen_fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP, NULL) == false) {
		log_err("failed to register listen fd\n");
		return false;
	}
	return true;
}

void TCPServer::do_poll()
{
	int events, i;
	int timeout; /* seconds */
	struct epoll_event cev[MAX_EVENTS];
	time_t start_time, end_time;

	timeout = loop_timeout;
	/* wait for events */
	while(!should_i_quit()) {
		log_debug("epoll_wait\n");
		start_time = get_monotonic_time();
		if ((events = epoll_wait(epoll_fd, cev, MAX_EVENTS, timeout*1000)) == -1) {
			log_err("epoll_wait() failed: %s\n", strerror(errno));
		} else {
			log_debug("Got %d events on rfd\n", events);
			for (i = 0; i < events; i++) {
				if(cev[i].data.ptr == NULL) {
					/* this is listen fd */
					accept_connection();
				} else {
					/* this is client fd */
					handle_event(&cev[i]);
				}
			} /* end FOR loop */
		}
		cmgr.free_invalid_clients();
		/* rework timeout */
		end_time = get_monotonic_time();
		timeout = timeout - (end_time - start_time);
		if(timeout <= 0) {
			house_keeping();
			timeout = EPOLL_LOOP_TIMEOUT;
		}
	} /* end WHILE loop */
}
