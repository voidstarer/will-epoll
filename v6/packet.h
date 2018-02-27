#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>
#include <arpa/inet.h>

/* struct _packet should be packed */
struct __attribute__((__packed__)) _packet {
	uint32_t type;    /* packet type - to identify the type of data being sent within the body */
	uint16_t src_id;  /* sender’s id in network order */
	uint16_t dst_id;  /* receiver’s id in network order */
	uint32_t length;  /* length of the packet (header + body ) in network order */
	uint8_t body[0];  /* contiguous array of bytes */
};

#define PACKET_HDR_LENGTH sizeof(struct _packet)

/* packet length should always be in network order */
/* total packet length HDR + BODY */
#define GET_PACKET_LENGTH(_p)      (ntohl(((struct _packet*)_p)->length))
#define GET_PACKET_BODY_LENGTH(_p) (GET_PACKET_LENGTH(_p) - PACKET_HDR_LENGTH)
#define GET_PACKET_TYPE(_p)        (ntohl(((struct _packet *)_p)->type))
#define GET_PACKET_SRC_ID(_p)      (ntohs(((struct _packet *)_p)->src_id))
#define GET_PACKET_DST_ID(_p)      (ntohs(((struct _packet *)_p)->dst_id))

/* one should always use this routine to set the packet length, the argument is 
 * packet and body length
 */
#define SET_PACKET_BODY_LENGTH(_p, _l)  { ((struct _packet *)_p)->length = htonl((_l) + PACKET_HDR_LENGTH); }
#define SET_PACKET_TYPE(_p, _l)         { ((struct _packet *)_p)->type = htonl(_l); }
#define SET_PACKET_SRC_ID(_p, _l)       { ((struct _packet *)_p)->src_id = htons(_l); }
#define SET_PACKET_DST_ID(_p, _l)       { ((struct _packet *)_p)->dst_id = htons(_l); }


#define MAX_PACKET_LENGTH (100 * 1024) /* 100 KB should be sufficient */
#define MAX_ALLOWED_WRITE_QUEUE (50 * MAX_PACKET_LENGTH) /* max 50 maximized packets can be queued */

typedef enum {
	PKT_HELLO=1,
	PKT_DATA,
	PKT_MAX
} packet_type;

#endif
