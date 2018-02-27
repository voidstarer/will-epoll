#include <cstdlib>
#include <cstring>
#include "packet.hpp"
	
Packet::Packet(struct _packet *p)
{
	type = (packet_type)GET_PACKET_TYPE(p);
	src_id = GET_PACKET_SRC_ID(p);
	dst_id = GET_PACKET_DST_ID(p);
	body_length = GET_PACKET_BODY_LENGTH(p);
	body = (uint8_t *)malloc((body_length+1) * sizeof(uint8_t));
	memcpy(body, p->body, body_length);
	body[body_length] = '\0'; /* just for the heck of it */
}

Packet::Packet(packet_type t, uint16_t s, uint16_t d, const uint8_t *b, uint32_t bl)
{
	type = t;
	src_id = s;
	dst_id = d;
	body_length = bl;
	body = (uint8_t *)malloc((body_length+1) * sizeof(uint8_t));
	memcpy(body, b, body_length);
	body[body_length] = '\0'; /* just for the heck of it */
}

Packet::~Packet()
{
	if(body) {
		free(body);
	}
}

const char *Packet::to_string()
{
	return (char*)body;
}

uint32_t Packet::get_serialized_length()
{
	return PACKET_HDR_LENGTH+body_length;
}

void Packet::serialize(uint8_t *buffer)
{
	SET_PACKET_SRC_ID(buffer, src_id)
	SET_PACKET_DST_ID(buffer, dst_id)
	SET_PACKET_TYPE(buffer, type);
	SET_PACKET_BODY_LENGTH(buffer, body_length);
	memcpy(((struct _packet *)buffer)->body, body, body_length);
}
