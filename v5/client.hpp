#ifndef _CLIENT_HPP_
#define _CLIENT_HPP_

#include <cstdint>

struct _read_buffer {
	uint32_t read_offset;
	uint32_t total_size;
	uint8_t *buffer;
};

struct _write_buffer {
	int32_t write_pending;
	int32_t total_size;
	uint8_t *buffer;
};


class Client {
public:
	uint16_t id;			/* client identifier */
	int32_t fd;			/* socket descriptor of the client */
	bool  invalid;		/* means that this connection is stale */
	bool  write_registered;	/* means the client is registered for write events */
	struct _read_buffer read_buffer;
	struct _write_buffer write_buffer;
	char ip[20];			/* ipv4 address in textual format */
	int32_t port;
};

#endif
