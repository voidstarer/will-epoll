#include "packet.hpp"
#include "log.hpp"

/* returns true if the entire header has been read */
bool Client::read_n_bytes(uint32_t n, int &err)
{
	int rc;
	rc = read(fd, read_buffer+read_offset, n);
	if(rc == 0) {
		/* connection closed */
		err = ESHUTDOWN;
		return false;
	}
	if(rc < 0) {
		if(errno == EINTR || errno == EAGAIN || errno == EINPROGRESS) {
			/* we will get called again from EPOLL */
			err = EAGAIN;
		} else {
			/* read sets this errno to indicate the error */
			err = errno;
		}
		return false;
	}
	read_offset += rc;
	if(rc < n) {
		/* we have less than required data, need to retry */
		err = EAGAIN;
		return false;
	}
	/* we have the required data */
	err = 0;
	return true;
}

/* returns true : if the entire header is read
 * 	   false: if less data was read, or connection was blocked or if an error was encountered
 *		  In case of false, it sets the err appropriately
 */
bool Client::read_header(int &err)
{
	uint32_t remaining;
	assert(read_offset < PACKET_HDR_LENGTH);
	remaining = PACKET_HDR_LENGTH-read_offset;
	return read_n_bytes(remaining, err);
}

/* returns true : if the entire body is read
 * 	   false: if less data was read, or connection was blocked or if an error was encountered
 *		  In case of false, it sets the err appropriately
 */
bool Client::read_body(int &err)
{
	uint32_t remaining;
	assert(read_offset >= PACKET_HDR_LENGTH);
	assert(GET_PACKET_LENGTH(read_buffer) <= MAX_PACKET_LENGTH);

	remaining = GET_PACKET_LENGTH(read_buffer) - read_offset;
	assert(remaining > 0);
	return read_n_bytes(remaining, err);
}

bool Client::validate_packet(int &err)
{
	struct _packet *p = (struct _packet *) read_buffer;
	/* convert from network order to host order here */
	if(GET_PACKET_SRC_ID(p) == UNIDENTIFIED_CLIENT) {
		/* the src id should always be known */
		log_err("Client: %hu fd: %d sent src_id\n", client->id, client->fd);
		err = EINVAL;
		return false;
	}
	if(id == UNIDENTIFIED_CLIENT) {
		/* this is the first packet and we should mark our id with it */
		id = GET_PACKET_SRC_ID(p);
	} else {
		/* src_id should always match our id */
		if(GET_PACKET_SRC_ID(p) != id) {
			err = EINVAL;
			log_err("id: %hu sent an invalid src_id: %hu\n", id, GET_PACKET_SRC_ID(p));
			return false;
		}
	}
	log_info("received packet: type: %u src_id:%hu dst_id:%hu length: %lu\n",
		GET_PACKET_TYPE(p), GET_PACKET_SRC_ID(p),
		GET_PACKET_DST_ID(p), GET_PACKET_DATA_LENGTH(p));

	switch(GET_PACKET_TYPE(p)) {
		case PKT_HELLO:
			/* this is a hello packet sent by the client to identify itself */
			/* control packets like HELLO should be consumed here itself */
			id = GET_PACKET_SRC_ID(p);
			log_info("id: %hu sent HELLO. Packet consumed internally\n", id);
			/* tell the caller that we have consumed this packet */
			err = EAGAIN;
			return false;
			break;
		case PKT_DATA:
			/* use this type to send data to another client */
			log_info("id: %hu => %hu:: '%.*s'\n", id, GET_PACKET_DST_ID(p),
				(int)GET_PACKET_DATA_LENGTH(p), packet->data);
			err = 0;
			break;
		default:
			/* client sent an invalid packet */
			log_err("id: %hu sent an invalid packet\n", id);
			err = EINVAL;
			return false;
			break;
	}

	return true;
}

void Client::reset_read_buffer()
{
	read_offset = 0;
}


/* returns new Packet in case of a valid packet received
 * returns NULL and sets the err to following:
 *	  ESHUTDOWN: in case of connection closed
 *	  EMSGSIZE: in case client sent an unexpectely large packet
 *	  EINVAL: in case of invalid packet sent by client
 *	  0 in the case of a packet read successfully
 *	  EAGAIN in the case of partial packet read or connection blocked (EINTR, EAGAIN, EINPROGRESS or EWOULDBLOCK)
 *	  EAGAIN is also set in the case of an internal packet type like HELLO packet
*/
Packet* Client::read_packet(int &err)
{
	Packet *P;
	if(read_offset < PACKET_HDR_LENGTH) {
		/* get the header first */
		if(read_header(err) == false)  {
			return NULL;
		}
		if(GET_PACKET_LENGTH(read_buffer) > MAX_PACKET_LENGTH) {
			err = EMSGSIZE;
			return NULL;
		}
	}
	if(read_body(err) == false) {
		return NULL;
	}
	/* here we have received a full packet, therefore,
	 * below this, we should always reset the read_buffer before return
	 */
	if(validate_packet(err) == false) {
		reset_read_buffer();
		return NULL;
	}

	/* create a new Packet and return that */
	P = new Packet((struct _packet *)read_buffer);
	reset_read_buffer();
	return P;
}

/*
 * Returns false: if packet could not be queued
 *	   true : if the packet was queueud
 */
bool Client::queue_packet(Packet *P)
{
	uint32_t rs;

	/* get required size */
	rs = P->get_serialized_length() + write_pending;
	if(rs > MAX_ALLOWED_WRITE_QUEUE) {
		log_err("Can't queue any more, write queue exceeds the given limit %d\n", MAX_ALLOWED_WRITE_QUEUE);
		return false;
	}
	P->serialize(write_buffer+write_pending);
	write_pending += P->get_serialized_length();
	log_debug("append %u bytes to write queue of id %hu, therefore, write_pending: %u\n",
				P->get_serialized_length(), id, write_pending);

	return true;
}

/*
 * Returns false in case of write failure
 *         true if write succeeds or connection blocks 
 */
bool Client::write()
{
	int wc;

	/* usually writes are slower than read, so let's loop it out, anyways the fd is non-blocking */
	for(;;) {
		wc = write(fd, write_buffer, write_pending);
		if(wc > 0) {
			/* subtract the data that was sent to client */
			write_pending -= wc;
			log_debug("written %d bytes to id %d, pending: %d\n", wc, client->id, write_pending);
			if(write_pending == 0) {
				/* we can unregister_write here, but instead it's better that we take one more call
				 * of EPOLLOUT and see if there is anything pending to write, because there are chances
				 * that in this epoll loop some-one might add something to this client's queue */
				break;
			} else {
				/* this will get rarely called */
				memmove(write_buffer, write_buffer+wc, write_pending);
			}
		} else {
			/* write should never return 0, so this is a case of -1 */
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS) {
				/* epoll will call us back */
				log_info("write blocked for id %hu\n", id);
				break;
			}
			log_info("write failed for id %hu: error: %s\n", id, strerror(errno));
			return false;
			break;
		}
	}
	return true;
}

