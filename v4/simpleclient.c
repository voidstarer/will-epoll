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
#include "client.h"

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

static void run_client(const char *hostname, int32_t server_port, uint16_t client_id, uint16_t dst_id)
{
    int count=1000000;
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char buf[BUFSIZE];
    struct _packet *packet;

    portno = server_port;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (connect(sockfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");

    for(int i=0; i<count; i++) {
	packet = (struct _packet *)buf;
	SET_PACKET_TYPE(packet, 5);
	SET_PACKET_SRC_ID(packet, client_id);
	SET_PACKET_DST_ID(packet, dst_id);
	n = snprintf((void*)packet->data, BUFSIZE-PACKET_HDR_LENGTH, "Hello %d from client %hu to %hu", i, client_id, dst_id);
	SET_PACKET_DATA_LENGTH(packet, n);

        fprintf(stdout, "sending packet: type: %u src_id:%hu dst_id:%hu length: %lu total_length=%u count=%d\n",
                GET_PACKET_TYPE(packet), GET_PACKET_SRC_ID(packet),
                GET_PACKET_DST_ID(packet), GET_PACKET_DATA_LENGTH(packet), GET_PACKET_LENGTH(packet), i);
	/* send the message line to the server */
	n = write(sockfd, packet, GET_PACKET_LENGTH(packet));
	if (n < 0) {
		error("ERROR writing to socket");
	}
//	sleep(1);

#if 0
	bzero(buf, BUFSIZE);
	n = read(sockfd, buf, BUFSIZE);
	if (n < 0) 
		error("ERROR reading from socket");
	printf("Echo from server: %s", buf);
#endif
    }

    close(sockfd);
    return;
}

static void usage(const char *name)
{
        fprintf(stderr, "usage: %s -s <server-addr> -i <client-id> -d <dest-client-id> [ -p <port>]\nDefault Port=2222\n", name);
        exit(1);
}

int main(int argc, char **argv)
{
	int optch;
	int32_t server_port=2222;
	uint16_t client_id, dst_id;
	const char *server_addr;

	client_id = dst_id = UNIDENTIFIED_CLIENT;

	while ((optch = getopt(argc, argv, "s:p:i:d:h")) != EOF) {
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
			case 'h':
			default:
				usage(argv[0]);
				break;
		}
	}
	if(server_addr == NULL || client_id == UNIDENTIFIED_CLIENT || dst_id == UNIDENTIFIED_CLIENT) {
		usage(argv[0]);
	}

	run_client(server_addr, server_port, client_id, dst_id);
        return(0);
}


