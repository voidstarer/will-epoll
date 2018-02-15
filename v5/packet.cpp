#include <cstdlib>
#include <cstring>
#include "packet.hpp"
	
Packet::Packet(struct _packet *p)
{
	type = (packet_type)GET_PACKET_TYPE(p);
	src_id = GET_PACKET_SRC_ID(p);
	dst_id = GET_PACKET_DST_ID(p);
	length = GET_PACKET_LENGTH(p);
	data = (uint8_t *)malloc((length+1) * sizeof(uint8_t));
	memcpy(data, p->data, p->length);
	data[length] = '\0'; /* just for the heck of it */
}

Packet::~Packet()
{
	if(data) {
		free(data);
	}
}

const char *Packet::to_string()
{
	return (char*)data;
}

