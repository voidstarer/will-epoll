#include <iostream>
#include <cstring>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>

#include "log.hpp"
#include "tcp_client.hpp"

bool TCPClient::register_read(Client *client)
{
	assert(client != NULL);
	log_info("called for id %d fd %d\n", client->id, client->fd);
	return register_event(client->fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP, client);
}

bool TCPClient::register_write(Client *client)
{
	assert(client != NULL);
	if(client->write_registered == true) {
		return true;
	}
	client->write_registered = true;
	log_info("called for id %d fd %d\n", client->id, client->fd);
	return register_event(client->fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP, client);
}

bool TCPClient::unregister_write(Client *client)
{
	assert(client != NULL);
	if(client->write_registered == false) {
		return true;
	}
	client->write_registered = false;
	log_info("called for id %d fd %d\n", client->id, client->fd);
	return register_event(client->fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP, client);
}

bool TCPClient::unregister_read(Client *client)
{
	assert(client != NULL);

	log_info(" id: %u fd: %d\n", client->id, client->fd);

	return register_event(client->fd, EPOLL_CTL_DEL, 0, NULL);
}

void TCPClient::ignore_sigpipe()
{
	struct sigaction ac;
	/* ignore sigpipe, let write get fail */
	memset(&ac, 0, sizeof ac);
	sigaction(SIGPIPE, NULL, &ac);
	ac.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &ac, NULL);
}

time_t TCPClient::get_monotonic_time()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		return 0;
	}
	return ts.tv_sec;
}

void TCPClient::handle_read_event(void *ptr)
{
	Client *client = (Client*)ptr;
	Packet *P;
	int ret;

	assert(client != NULL);
	assert(client->invalid == false);
	log_info("id: %hu fd: %d\n", client->id, client->fd);

	P = client->read_packet(ret);
	if(P) {
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
		log_info(" id: %hu: packet internally consumed\n", client->id);
	} else {
		if(ret == EAGAIN) {
			/* epoll will bring us back */
			log_info(" id: %hu returning back to epoll\n", client->id);
			return;
		} else if(ret == EMSGSIZE) {
			log_err( " received a large message\n");
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
		disconnect();
	}
}

bool TCPClient::send_data_to_server(Packet *P)
{
	Client *client = me;
	assert(me != NULL);
	client->queue_packet(P);
	return register_write(client);
}

void TCPClient::handle_write_event(void *ptr)
{
	Client *client = (Client*)ptr;
	assert(client != NULL);
	assert(client->invalid == false);

	log_info("called for id: %u fd: %d\n", client->id, client->fd);

	if(client->has_write_pending() == false) {
		log_debug("id: %hu nothing more to write, will remove write registration\n", client->id);
		unregister_write(client);
		return;
	}

	if(client->write_data() == false) {
		/* terrible error, free the client */
		on_error(client, errno);
		disconnect();
	}

	return;
}


void TCPClient::handle_event(const struct epoll_event *eptr)
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

bool TCPClient::check_connect(uint32_t events, int fd)
{
	uint32_t error=0, len=sizeof(error);

	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		log_err("getsockopt(%d) failed '%s'\n", fd, strerror(errno));
		return false;
	}
	if (error) {
		log_err("connect(%s:%d) failed '%s'\n", server_ip, server_port, strerror(error));
		return false;
	}
	return true;
}

void TCPClient::disconnect()
{
	log_info("called\n");
	assert(state != DISCONNECTED);
	close(server_fd);
	server_fd = -1;
	state=DISCONNECTED;
	delete me;
	me = NULL;
}

void TCPClient::send_packet(packet_type type, uint16_t dst_id, const uint8_t *data, uint32_t data_len)
{
	Packet *P;
	P = new Packet(type, id, dst_id, data, data_len);
	send_data_to_server(P);
	delete P;
}


void TCPClient::send_hello()
{
	send_packet(PKT_HELLO, 0, NULL, 0);
}

void TCPClient::handle_initial_state(const struct epoll_event *eptr)
{
	switch(state) {
		case CONNECTING:
			if(check_connect(eptr->events, server_fd) == false) {
				disconnect();
				return;
			}
			log_info("connected successfully to server %s:%d. Next step: handshake\n", server_ip, server_port);
			state = CONNECTED;
			me = new Client(server_fd);
			me->id = id;
			strcpy(me->ip, server_ip);
			me->port = server_port;
			me->mode = MODE_CLIENT;
			on_connect(me);
			send_hello();
			break;
		case CONNECTED:
		case DISCONNECTED:
		default:
			log_err("invalid state %d", state);
			assert(NULL && "we should never reach here");
			break;
	};
}

bool TCPClient::connect_to_server()
{
	struct sockaddr_in sin;
	int fd;

	log_info("%s:%d\n", server_ip, server_port);
	assert(state == DISCONNECTED);
	if(inet_pton(AF_INET, server_ip, &(sin.sin_addr)) <= 0) {
		log_err("invalid addr %s\n", server_ip);
		return false;
	}
	sin.sin_family = AF_INET;
	sin.sin_port = htons(server_port);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_err("socket() failed '%s'\n", strerror(errno));
		return false;
	}
	set_socket_nonblocking(fd);
	if (connect(fd, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)) < 0) {
		if ((errno != EAGAIN) && (errno != EINPROGRESS)) {
			log_err("connect(%s:%d) failed '%s'\n", server_ip, server_port, strerror(errno));
			close(fd);
			return false;
		}
	}
	state = CONNECTING;
	server_fd = fd;
	return register_event(fd, EPOLL_CTL_ADD, EPOLLOUT | EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP, NULL);
}

bool TCPClient::initialize()
{
	ignore_sigpipe();

	if(Event::initialize() == false) {
		return false;
	}

	if(connect_to_server() == false) {
		log_err("failed to connect to server\n");
		return false;
	}

	return true;
}

void TCPClient::do_poll()
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
					/* this is server fd before the connection/handshake */
					handle_initial_state(&cev[i]);
				} else {
					/* this is other fd */
					handle_event(&cev[i]);
				}
			} /* end FOR loop */
		}
		/* rework timeout */
		end_time = get_monotonic_time();
		timeout = timeout - (end_time - start_time);
		if(timeout <= 0) {
			if(state == DISCONNECTED) {
				/* retry connection */
				connect_to_server();
			}
			house_keeping();
			timeout = EPOLL_LOOP_TIMEOUT;
		}
	} /* end WHILE loop */
}
