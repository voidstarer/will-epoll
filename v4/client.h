#ifndef _CLIENTS_H_
#define _CLIENTS_H_

#define MAX_CLIENTS (65536) /* 16 bit max */
#define UNIDENTIFIED_CLIENT (MAX_CLIENTS-1)


#include <stdint.h>
#include <arpa/inet.h>

/* struct _packet should be packed */
struct __attribute__((__packed__)) _packet {
	uint32_t type;    /* packet type - to identify the type of data being sent within */
	uint16_t src_id;  /* sender’s id in network order */
	uint16_t dst_id;  /* receiver’s id in network order */
	uint32_t length;  /* length of the packet (header + data ) in network order */
	uint8_t data[0];  /* contiguous array of bytes */
};

#define PACKET_HDR_LENGTH sizeof(struct _packet)

/* packet length should always be in network order */
/* total packet length HDR + DATA */
#define GET_PACKET_LENGTH(_p)      (ntohl(((struct _packet*)_p)->length))
#define GET_PACKET_DATA_LENGTH(_p) (GET_PACKET_LENGTH(_p) - PACKET_HDR_LENGTH)
#define GET_PACKET_TYPE(_p)        (ntohl(((struct _packet *)_p)->type))
#define GET_PACKET_SRC_ID(_p)      (ntohs(((struct _packet *)_p)->src_id))
#define GET_PACKET_DST_ID(_p)      (ntohs(((struct _packet *)_p)->dst_id))

/* one should always use this routine to set the packet length, the argument is 
 * packet and data length
 */
#define SET_PACKET_DATA_LENGTH(_p, _l)  { ((struct _packet *)_p)->length = htonl((_l) + PACKET_HDR_LENGTH); }
#define SET_PACKET_TYPE(_p, _l)         { ((struct _packet *)_p)->type = htonl(_l); }
#define SET_PACKET_SRC_ID(_p, _l)       { ((struct _packet *)_p)->src_id = htons(_l); }
#define SET_PACKET_DST_ID(_p, _l)       { ((struct _packet *)_p)->dst_id = htons(_l); }


#define MAX_PACKET_LENGTH (100 * 1024) /* 100 KB should be sufficient */
#define MAX_ALLOWED_WRITE_QUEUE (10 * MAX_PACKET_LENGTH) /* max 10 maximized packets can be queued */

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

struct _client {
	uint16_t id;			/* client identifier */
	uint8_t  invalid;		/* means that this connection is stale */
	uint32_t fd;			/* socket descriptor of the client */
	struct _read_buffer read_buffer;
	struct _write_buffer write_buffer;
	char ip[20];			/* ipv4 address in textual format */
	int32_t port;
};

//#define __str(a) #a
//#define xstr(a) __str(a)
#define xstr(a)

/* prototype */
struct _client *new_client(int fd);
void mark_client_as_invalid(struct _client *client);
void add_new_client_to_array(struct _client *client, uint16_t id);
void free_invalid_clients();
struct _client *find_client_by_id(uint16_t id);
int send_data_to_client(uint16_t dst_id, struct _packet *packet);


#endif
