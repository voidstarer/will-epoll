#ifndef _CLIENT_HPP_
#define _CLIENT_HPP_

#include <cstdint>
#include "unistd.h"

#define MAX_CLIENTS (65536) /* 16 bit max */
#define UNIDENTIFIED_CLIENT (MAX_CLIENTS-1)

#define MAX_PACKET_LENGTH (100 * 1024) /* 100 KB should be sufficient */
#define MAX_ALLOWED_WRITE_QUEUE (50 * MAX_PACKET_LENGTH) /* max 50 maximized packets can be queued */

class Client {
public:
	uint16_t id;				/* client identifier */
	int32_t fd;				/* socket descriptor of the client */
	bool  invalid;				/* means that this connection is stale */
	bool  write_registered;			/* means the client is registered for write events */
	uint32_t read_offset;
	uint8_t read_buffer[MAX_PACKET_LENGTH];
	uint32_t write_pending;
	uint8_t write_buffer[MAX_ALLOWED_WRITE_QUEUE];
	char ip[20];				/* ipv4 address in textual format */
	int32_t port;
	Client(int fd) : 
			id(UNIDENTIFIED_CLIENT), fd(fd),
			invalid(false), write_registered(false),
			read_offset(0), read_buffer(), write_pending(0),
			write_buffer(), ip(), port(0)
	{ }

	~Client()
	{
		if(fd >= 0) {
			close(fd);
		}
	}
};

#endif
