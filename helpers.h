#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN 1650	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare
#define TOPIC_SIZE 50 
#define DATA_SIZE 1500
#define IP_SIZE 16
#define TYPE_SIZE 12
#define CLIENT_ID_LEN 10
#define PORT_LEN 16

#define CLIENT_ID_CODE 0
#define SUBSCRIBE_CODE 1
#define UNSUBSCRIBE_CODE 2

#define UDP_MESSAGE_LEN TOPIC_SIZE + DATA_SIZE + 2
#define PAYLOAD_LEN TOPIC_SIZE + 2
#define MESSAGE_MAX_LEN PAYLOAD_LEN + 2
 
#define ONLINE true
#define OFFLINE false

struct message_udp {
	char topic[TOPIC_SIZE];
	uint8_t data_type;
	char data[DATA_SIZE + 1];
};

struct client_info {
	int sock;
	bool status;
};

struct message {
	uint8_t len;
	// CLIENT_ID_CODE, SUBSCRIBE_CODE, UNSUBSCRIBE_CODE (client->server)
	uint8_t code; 
	char payload[PAYLOAD_LEN];
};

struct message_to_server {
	char topic[TOPIC_SIZE + 1];
	bool SF;
};

#endif
