#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "client.h"
#include "log.h"

#define MAX_EVENTS 100

static int epoll_fd = -1;

extern void onConnect(struct _client *client);
extern void onDataReceived(struct _client *client, struct _packet *packet);
extern void onDisconnect(struct _client *client);
extern void onError(struct _client *client, int32_t error_code);
extern void houseKeeping();

/*
 * Set the socket to non blocking 
 */
static int
set_socket_nonblocking(int sock)
{
	int flags;

	assert(sock >= 0);

	flags = fcntl(sock, F_GETFL, 0);
	return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

static int
bind_port(int port)
{
	struct sockaddr_in addr;
	int fd;
	const int on = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr("0.0.0.0");

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		log_err(" Unable to bind listening socket to port %d because of %s\n",
				port, strerror(errno));
		close(fd);
		return -1;
	}

	if (listen(fd, 20) < 0) {
		log_err("Unable to start listening socket because of %s\n", strerror(errno));
		close(fd);
		return -1;
	}
	set_socket_nonblocking(fd);

	return fd;
}

static int
register_read_fd(int fd, uint32_t events, void *data_ptr)
{
	struct epoll_event ev;

	assert(fd >= 0);
	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.ptr = (void *)data_ptr;

	log_debug(" fd: %d\n", fd);

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1 ) {
		log_err(" epoll_ctl failed: %s\n", strerror(errno));
		return -1;
	}
	return 0;

}

static int
register_read(struct _client *client)
{
	assert(client != NULL);
	return register_read_fd(client->fd, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP, client);
}

int
unregister_read(struct _client *client)
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
		return -1;
	}
	return 0;
}


static void
accept_connection(int fd)
{
	int newfd;
	socklen_t size;
	struct sockaddr_in cl;
	struct _client *client;

	size = sizeof(struct sockaddr_in);
	newfd = accept(fd, (struct sockaddr *)&cl, &size);
	if (newfd < 0) {
		log_err("accept() failed: %s\n", strerror(errno));
		return;
	}

	set_socket_nonblocking(newfd);
	client = new_client(newfd);
	inet_ntop(AF_INET, &cl.sin_addr, client->ip, sizeof(client->ip));
	client->port = ntohs(cl.sin_port);

	/* onConnect */
	onConnect(client);
	register_read(client);
	return;
}

#define EPOLL_LOOP_TIMEOUT 40 /* seconds */

static void 
ignore_sigpipe()
{
	struct sigaction ac;
	/* ignore sigpipe, let write get fail */
	memset(&ac, 0, sizeof ac);
	sigaction(SIGPIPE, NULL, &ac);
	ac.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &ac, NULL);
}

static time_t
get_monotonic_time()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		return 0;
	}
	return ts.tv_sec;
}

static int
validate_packet(struct _client *client, struct _packet *packet)
{
	/* convert from network order to host order here */
	if(GET_PACKET_SRC_ID(packet) == UNIDENTIFIED_CLIENT) {
		/* the src id should always be known */
		return -1;
	}
	log_info("received packet: type: %u src_id:%hu dst_id:%hu length: %lu\n",
		GET_PACKET_TYPE(packet), GET_PACKET_SRC_ID(packet),
		GET_PACKET_DST_ID(packet), GET_PACKET_DATA_LENGTH(packet));

	return 0;
}

static int process_packets(struct _client *client)
{
	struct _packet *packet;
	uint32_t packet_len;
	struct _read_buffer *rbuf = &client->read_buffer;
	
	log_debug("id %hu\n", client->id);
	for(;;) {
		/* handle one packet at a time */
		log_debug("id %hu read_offset=%u\n", client->id, rbuf->read_offset);
		if(rbuf->read_offset < PACKET_HDR_LENGTH) {
			return EAGAIN;
		}
		packet = (struct _packet *)rbuf->buffer;
		packet_len = GET_PACKET_LENGTH(packet);
		log_debug("id %hu packet_len=%u\n", client->id, packet_len);
		if(rbuf->read_offset < packet_len) {
			return EAGAIN;
		}
		if(validate_packet(client, packet) != 0) {
			log_err(" id %d sent invalid packet\n", client->id);
			return EINVAL;
		}
		/* we have a good packet */
		if(client->id == UNIDENTIFIED_CLIENT) {
			/* the same client pointer will remain, the older one should be freed here */
			add_new_client_to_array(client, GET_PACKET_SRC_ID(packet));
		}

		onDataReceived(client, packet);
		/* onDateReceived should consume the packet by duplicating it */
		rbuf->read_offset -= packet_len;

		if(rbuf->read_offset == 0) {
			break;
		}
		memmove(rbuf->buffer, rbuf->buffer+packet_len, rbuf->read_offset); 
	}
	return EAGAIN;
}

#define MIN_BUFFER_SIZE 1024

/* return -1  in the case of error
 *	  ESHUTDOWN: in case of connection closed
 *	  EMSGSIZE: in case client sent an unexpectely large packet
 *	  0 in the case of a packet read successfully
 *	  EAGAIN in the case of partial packet read or connection blocked (EINTR, EAGAIN, EINPROGRESS or EWOULDBLOCK)
*/
int read_packets(struct _client *client)
{
	struct _read_buffer *rbuf = &client->read_buffer;
	int rc;

	/* before reading, check if we have enough space in buffer */
	if(rbuf->read_offset > 0) {
		/* we have some data which was read previously */
		/* check we have the header of the packet, which will allow us
		 * to predict the size of buffer which we will need over here
		 */
		if(rbuf->read_offset >= PACKET_HDR_LENGTH) {
			/* assert that the previous packets have been consumed before we read any new one */
			assert(rbuf->read_offset < GET_PACKET_LENGTH(rbuf->buffer));
			/* after read, we have already checked if the packet exceeds the MAX_PACKET_LENGTH,
			   so assert it here just to catch any memory buffer overruns */
			assert(GET_PACKET_LENGTH(rbuf->buffer) <= MAX_PACKET_LENGTH);
			/* now, check if we have enough space to read the entire packet */
			if(rbuf->total_size < GET_PACKET_LENGTH(rbuf->buffer)) {
				rbuf->buffer = realloc(rbuf->buffer, GET_PACKET_LENGTH ( rbuf->buffer ));
				rbuf->total_size = GET_PACKET_LENGTH(rbuf->buffer);
			}

		} else {
			/* we don’t have the header yet, and but we should definitely have the room for it
			 * coz first time allocation happens of size MIN_BUFFER_SIZE
			 */
			assert(rbuf->total_size > PACKET_HDR_LENGTH);
		}
	} else {
		/* a new packet is going to arrive */
		if(rbuf->total_size == 0) {
			/* first allocation of buffer */
			rbuf->buffer = malloc(MIN_BUFFER_SIZE);
			rbuf->total_size = MIN_BUFFER_SIZE;
		} else {
			assert(rbuf->total_size > PACKET_HDR_LENGTH);
		}
	}

	/* now we have some room to read packets */
	rc = read(client->fd, rbuf->buffer + rbuf->read_offset, rbuf->total_size - rbuf->read_offset);
	if(rc == 0) {
		/* connection closed */
		return ESHUTDOWN;
	}
	if(rc < 0) {
		if(errno == EINTR || errno == EAGAIN || errno == EINPROGRESS) {
			/* we will get called again from EPOLL */
			return EAGAIN;
		}
		/* read sets this errno to indicate the error */
		return -1;
	}
	rbuf->read_offset += rc;
	log_info("read() returned %d bytes read_offset=%u\n", rc, rbuf->read_offset);
	/* rc > 0 we have a positive condition here */
	if(rbuf->read_offset >= PACKET_HDR_LENGTH) {
		if(GET_PACKET_LENGTH(rbuf->buffer) > MAX_PACKET_LENGTH) {
			return EMSGSIZE;
		}
		if(rbuf->read_offset >= GET_PACKET_LENGTH(rbuf->buffer)) {
			/* we have atleast one full packet available, it may have multiple packets */
			return 0;
		}
		/* we need to retry to get the full packet */
	}
	return EAGAIN;
}

static void handle_read_event(void *ptr)
{
	struct _client *client = ptr;
	int ret;

	assert(client != NULL);
	if(client->invalid) {
		/* this is a stale client */
		log_err(" id: %u fd: %d is invalid\n", client->id, client->fd);
		return;
	}
	log_info("id: %hu fd: %d\n", client->id, client->fd);

	ret = read_packets(client);
	if(ret == 0) {
		/* we may have more than one packets to process */
		process_packets(client);
		return;
	} else if(ret == EAGAIN) {
		/* epoll will bring us back */
		log_info(" id: %hu returning back to epoll\n", client->id);
		return;
	} else if(ret == EMSGSIZE) {
		log_err( " id: %hu sent a large message\n", client->id);
		onError(client, EMSGSIZE);
		mark_client_as_invalid(client);
	} else if(ret == ESHUTDOWN) {
		onDisconnect(client);
		log_err(" id: %hu closed the connection\n", client->id);
		mark_client_as_invalid(client);
	} else {
		onError(client, errno);
		log_err(" id: %hu fd:%d read failed\n", client->id, client->fd);
		mark_client_as_invalid(client);
	}
}

static void handle_write_event(void *ptr)
{
	struct _client *client = ptr;
	assert(client != NULL);
	if(client->invalid) {
		/* this is a stale client */
		log_err(" id: %u fd: %d is invalid\n", client->id, client->fd);
		return;
	}
	log_info(" called for id: %u fd: %d\n", client->id, client->fd);
	assert(NULL && " we should not reach here\n");
}


static void handle_event(const struct epoll_event *eptr)
{
	uint32_t ev = eptr->events; 
	if (ev & EPOLLIN)    {log_info("EPOLLIN ");}
	if (ev & EPOLLPRI)   {log_info("EPOLLPRI ");}
	if (ev & EPOLLOUT)   {log_info("EPOLLOUT ");}
	if (ev & EPOLLERR)   {log_info("EPOLLERR ");}
	if (ev & EPOLLHUP)   {log_info("EPOLLHUP ");}
	if (ev & EPOLLRDHUP) {log_info("EPOLLRDHUP ");}

	if (ev & (EPOLLIN|EPOLLERR|EPOLLHUP|EPOLLRDHUP)) {
		handle_read_event(eptr->data.ptr);
	} else if (ev & (EPOLLOUT|EPOLLHUP|EPOLLERR)) {
		handle_write_event(eptr->data.ptr);
	} else {
		/* we should never reach here */
		assert(NULL && xstr(__func__) "we should never reach here");
	}
}

void
do_poll(int port)
{
	int listen_fd;
	int events, i;
	int timeout; /* seconds */
	struct epoll_event cev[MAX_EVENTS];
	time_t start_time, end_time;

	ignore_sigpipe();

	/* create a new epoll */
	if ((epoll_fd = epoll_create(MAX_EVENTS)) == -1) { 
		log_err("epoll_create: failed: %s\n", strerror(errno));
		exit(1);
	}

	listen_fd = bind_port(port);
	if(listen_fd < 0) {
		log_err("failed to create listen socket\n");
		exit(1);
	}

	/* listen_fd is explicitly registered with EPOLLIN only */
	if(register_read_fd(listen_fd, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP, NULL) != 0) {
		log_err("failed to register listen fd\n");
		exit(1);
	}

	timeout = EPOLL_LOOP_TIMEOUT;
	/* wait for events */
	while(1) {
		log_debug("epoll_wait\n");
		start_time = get_monotonic_time();
		if ((events = epoll_wait(epoll_fd, cev, MAX_EVENTS, timeout*1000)) == -1) {
			log_err("epoll_wait() failed: %s\n", strerror(errno));
		} else {
			log_debug("Got %d events on rfd\n", events);
			for (i = 0; i < events; i++) {
				if(cev[i].data.ptr == NULL) {
					/* this is listen fd */
					accept_connection(listen_fd);
				} else {
					/* this is client fd */
					handle_event(&cev[i]);
				}
			} /* end FOR loop */
		}
		free_invalid_clients();
		/* rework timeout */
		end_time = get_monotonic_time();
		timeout = timeout - (end_time - start_time);
		if(timeout <= 0) {
			houseKeeping();
			timeout = EPOLL_LOOP_TIMEOUT;
		}
	} /* end WHILE loop */
}
