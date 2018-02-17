/* 
 * tcpclient.c - A simple TCP client
 * usage: tcpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "../client.h"

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

static int connect_to_server(const char *hostname, int32_t server_port)
{
	int sockfd, portno;
	struct sockaddr_in serveraddr;
	struct hostent *server;

	portno = server_port;

	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("ERROR opening socket");
	}

	/* gethostbyname: get the server's DNS entry */
	server = gethostbyname(hostname);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host as %s\n", hostname);
		exit(1);
	}

	/* build the server's Internet address */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
	serveraddr.sin_port = htons(portno);

	/* connect: create a connection with the server */
	if (connect(sockfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
		error("ERROR connecting");
	}
	return sockfd;
}

static void
send_packet(int server_fd, uint16_t client_id, uint16_t dst_id, uint32_t type, const char *data)
{
	int n = 0;
	struct _packet *packet;
	char *buf;

	if(data) {
		n = strlen(data);
	}
	buf = calloc(1, n+PACKET_HDR_LENGTH);

	/* send a packet to identify ourselves to server */
	packet = (struct _packet *)buf;
	SET_PACKET_TYPE(packet, type);
	SET_PACKET_SRC_ID(packet, client_id);
	SET_PACKET_DST_ID(packet, dst_id);
	SET_PACKET_DATA_LENGTH(packet, n);
	memcpy((void*)packet->data, data, n);

	fprintf(stdout, "sending packet: type: %u src_id:%hu dst_id:%hu length: %lu total_length=%u\n",
		GET_PACKET_TYPE(packet), GET_PACKET_SRC_ID(packet),
		GET_PACKET_DST_ID(packet), GET_PACKET_DATA_LENGTH(packet), GET_PACKET_LENGTH(packet));
	/* send the message line to the server */
	n = write(server_fd, packet, GET_PACKET_LENGTH(packet));
	if (n != GET_PACKET_LENGTH(packet)) {
		error("ERROR writing to socket");
	}
	free(buf);
}

static void
send_hello(int server_fd, int client_id)
{
	send_packet(server_fd, client_id, 0, PKT_HELLO, NULL);
}

static void send_invalid_packet(int server_fd, int client_id, int dst_id)
{
	send_packet(server_fd, client_id, dst_id, PKT_MAX, NULL);
}


static void send_large_packet(int server_fd, int client_id, int dst_id)
{
	char buffer[100 * 1024];
	memset(buffer, 'a', sizeof(buffer));
	buffer[sizeof(buffer)-1] = '\0';
	send_packet(server_fd, client_id, dst_id, PKT_DATA, buffer);
}


static void
bombard_write(int server_fd, int client_id, int dst_id)
{
	int n;
	struct _packet *packet;
	char buf[BUFSIZE];
	int count=1000000;

	for(int i=0; i<count; i++) {
		packet = (struct _packet *)buf;
		SET_PACKET_TYPE(packet, PKT_DATA);
		SET_PACKET_SRC_ID(packet, client_id);
		SET_PACKET_DST_ID(packet, dst_id);
		n = snprintf((void*)packet->data, BUFSIZE-PACKET_HDR_LENGTH, "Hello %d from client %hu to %hu", i, client_id, dst_id);
		SET_PACKET_DATA_LENGTH(packet, n);

		fprintf(stdout, "sending packet: type: %u src_id:%hu dst_id:%hu length: %lu total_length=%u count=%d\n",
			GET_PACKET_TYPE(packet), GET_PACKET_SRC_ID(packet),
			GET_PACKET_DST_ID(packet), GET_PACKET_DATA_LENGTH(packet), GET_PACKET_LENGTH(packet), i);
		/* send the message line to the server */
		n = write(server_fd, packet, GET_PACKET_LENGTH(packet));
		if (n != GET_PACKET_LENGTH(packet)) {
			error("ERROR writing to socket");
		}
	}
	return;
}

static void bombard_read(int server_fd)
{
	int n, rc;
	struct _packet *packet;
	char buf[BUFSIZE];
	packet = (void*)buf;

	rc = 0;
	bzero(buf, BUFSIZE);
	while(1) {
		//printf("Waiting in read\n");
		n = read(server_fd, buf+rc, BUFSIZE-rc);
		if (n <= 0) {
			error("ERROR reading from socket");
		}

		//printf("Read returned %d bytes\n", rc);
		rc += n;
		for(;;) {
			//printf("rc=%d\n", rc);
			if(rc < PACKET_HDR_LENGTH) {
				break;
			}
			//printf("packet_length=%d\n", GET_PACKET_LENGTH(packet));
			if(rc < GET_PACKET_LENGTH(packet)) {
				break;
			}

			fprintf(stdout, "type: %u src_id:%hu dst_id:%hu length: %lu total_length=%u msg=%.*s\n",
				GET_PACKET_TYPE(packet), GET_PACKET_SRC_ID(packet),
				GET_PACKET_DST_ID(packet), GET_PACKET_DATA_LENGTH(packet), GET_PACKET_LENGTH(packet),
				(int)GET_PACKET_DATA_LENGTH(packet), (char*)packet->data);

			rc -= GET_PACKET_LENGTH(packet);
			memmove(buf, buf+GET_PACKET_LENGTH(packet), rc);
//                      sleep(1);
		}
	}
}


static void usage(const char *name)
{
        fprintf(stderr, "usage: %s -s <server-addr> -i <client-id> -d <dest-client-id> [-r][ -p <port>]\n"\
			"Default Port=2222\n-r is reader mode. Default is writer mode.\n", name);
        exit(1);
}

int main(int argc, char **argv)
{
	int server_fd;
	int optch;
	int reader_mode=0;
	int32_t server_port=2222;
	uint16_t client_id, dst_id;
	const char *server_addr;

	client_id = dst_id = UNIDENTIFIED_CLIENT;

	while ((optch = getopt(argc, argv, "s:p:i:d:hr")) != EOF) {
		switch(optch) {
			case 'p':
				server_port = atoi(optarg);
				break;
			case 's':
				server_addr = optarg;
				break;
			case 'i':
				client_id = atoi(optarg);
				break;
			case 'd':
				dst_id = atoi(optarg);
				break;
			case 'r':
				reader_mode = 1;
				break;
			case 'h':
			default:
				usage(argv[0]);
				break;
		}
	}
	if(server_addr == NULL || client_id == UNIDENTIFIED_CLIENT || dst_id == UNIDENTIFIED_CLIENT) {
		usage(argv[0]);
	}

	server_fd = connect_to_server(server_addr, server_port);
	send_hello(server_fd, client_id);

	if(reader_mode) {
		bombard_read(server_fd);
	} else {
		bombard_write(server_fd, client_id, dst_id);
		/* to cover the invalid code */
		/* wait for a while so that housekeeping gets covered */
		sleep(40);
		send_invalid_packet(server_fd, client_id, dst_id);
		send_large_packet(server_fd, client_id, dst_id);
	}
        return(0);
}

