#ifndef _PACKET_HPP_
#define _PACKET_HPP_

#include "packet.h"
/* prototype */

class Packet {
public:
	packet_type type;
	uint16_t src_id;
	uint16_t dst_id;
	uint32_t body_length;
	uint8_t *body;
	
	Packet(struct _packet *p);
	Packet(packet_type t, uint16_t s, uint16_t d, const uint8_t *b, uint32_t bl);

	~Packet();

	const char *to_string();
	void serialize(uint8_t *buffer);
	uint32_t get_serialized_length();
};

#endif
