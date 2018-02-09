#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "client.h"
#include "event.h"
#include "log.h"

static int quit=0;

void signal_handler(int signo)
{
	quit = 1;
}

int should_i_quit()
{
	return quit;
}

void onConnect(struct _client *client)
{
	printf("Client connected from ip %s and port %d\n", client->ip, client->port);
}

void onDataReceived(struct _client *client, struct _packet *packet)
{

	switch(GET_PACKET_TYPE(packet)) {
		case PKT_HELLO:
			/* this is a hello packet sent by the client to identify itself */
			fprintf(stdout, "id: %hu sent HELLO\n", client->id);
			break;
		case PKT_DATA:
			/* use this type to send data to another client */
			fprintf(stdout, "id: %hu => %hu:: '%.*s'\n", client->id, GET_PACKET_DST_ID(packet),
				(int)GET_PACKET_DATA_LENGTH(packet), packet->data);
			send_data_to_client(GET_PACKET_DST_ID(packet), packet);
			break;
		default:
			/* client sent an invalid packet */
			break;
	} 
}

void onDisconnect(struct _client *client)
{
	fprintf(stdout, "id %d Disconnected\n", client->id);
}

void onError(struct _client *client, int32_t error_code)
{
	fprintf(stdout, "Error on id %d: %s \n", client->id, strerror(error_code));
}

void houseKeeping()
{
	fprintf(stdout, "houseKeeping called\n");
}

static void usage(const char *name)
{
	fprintf(stderr, "usage: %s [ -p <port>][ -l 0|1|2 ]\nDefault Port=2222.\nLoglevel: 0 - Error, 1 - Info, 2 - Debug\n", name);
	exit(1);
}

int main(int argc, char **argv)
{
	int optch;
	int server_port=2222;
	int l=DEBUG;

	while ((optch = getopt(argc, argv, "p:l:h")) != EOF) {
		switch(optch) {
			case 'p':
				server_port = atoi(optarg);
				break;
			case 'l':
				l = atoi(optarg);
				break;
			case 'h':
			default:
				usage(argv[0]);
		}
	}
	setlinebuf(stdout);
	setlinebuf(stderr);
	set_log_level(l);
	signal(SIGINT, signal_handler);
	do_poll(server_port);
	return(0);
}
