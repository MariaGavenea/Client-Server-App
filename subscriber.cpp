#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <ID_Client> <IP_Server> <Port_Server>\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	char buffer1[BUFLEN];

	if (argc < 4) {
		usage(argv[0]);
	}

	if (strlen(argv[1]) > CLIENT_ID_LEN) {
		// lungime ID > 10
		fprintf(stderr, "Client's ID should have at most %d characters\n", CLIENT_ID_LEN);
		exit(0);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	// trimite ID-ul clientului catre server
	struct message m;
	memset(&m, 0, MESSAGE_MAX_LEN);

	m.code = CLIENT_ID_CODE;
	memcpy(m.payload, argv[1], strlen(argv[1]));
	m.len = MESSAGE_MAX_LEN;
  	n = send(sockfd, &m, m.len, 0);
  	DIE(n < 0, "send");

	fd_set read_fds;
	fd_set tmp_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// se adauga file descriptorii (socketul tcp si cel pentru date citite de la tastatura)
	// in multimea read_fds
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);

	while (1) {
		tmp_fds = read_fds;
		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
  			// se citeste de la tastatura
			memset(buffer1, 0, BUFLEN);
			fgets(buffer1, BUFLEN - 1, stdin);

			if (strncmp(buffer1, "exit\n", 5) == 0) {
				break;
			}

			struct message msg;
			memset(&msg, 0, MESSAGE_MAX_LEN);
			struct message_to_server *content = (struct message_to_server *) msg.payload;
			bool valid_message = false;

			char delim[] = " \n";
			char *ptr = strtok(buffer1, delim);			

			// subscribe este primul cuvant
			if (ptr != NULL && strncmp(ptr, "subscribe", 9) == 0) {
				// al doilea cuvant trebuie sa aiba lungimea maxim TOPIC_SIZE
				// al treilea cuv tb sa fie 0 sau 1
				int cnt = 1;
				bool valid_topic_size = true;
				bool valid_SF = true;

				if (strlen(ptr) == 9) {
					msg.code = SUBSCRIBE_CODE;

					while(ptr != NULL && cnt < 5) {
						int len = strlen(ptr);

						if (cnt == 2) {
							if (len > TOPIC_SIZE) {
								valid_topic_size = false;
							} else {
								memcpy(content->topic, ptr, len);
								content->topic[len] = '\0';
							}
						}

						if (cnt == 3) {
							if ((len != 1) || (len == 1 && ptr[0] != '0' && ptr[0] != '1')) {
								valid_SF = false;
							} else {
								content->SF = ptr[0] - '0';
							}
						}

						cnt++;
						ptr = strtok(NULL, delim);
					}
					cnt--;
				}

				if (cnt != 3) {
					fprintf(stderr, "Command form: subscribe <topic> <SF>\n");
					continue;
				}

				if (valid_topic_size == false) {
					fprintf(stderr, "Topic size should be <= %d\n", TOPIC_SIZE);
					continue;
				}

				if (valid_SF == false) {
					fprintf(stderr, "SF can only be 0 or 1\n");
					continue;
				}

				valid_message = true;
			}

			// unsubscribe este primul cuvant
			if (ptr != NULL && strncmp(ptr, "unsubscribe", 11) == 0) {
				// al doilea cuvant trebuie sa aiba lungimea maxim TOPIC_SIZE
				int cnt = 1;
				bool valid_topic_size = true;

				if (strlen(ptr) == 11) {
					msg.code = UNSUBSCRIBE_CODE;

					while(ptr != NULL && cnt < 4) {
						if (cnt == 2) {
							int len = strlen(ptr);
							if (len > TOPIC_SIZE) {
								valid_topic_size = false;
							} else {
								memcpy(content->topic, ptr, len);
								content->topic[len] = '\0';
							}
						}

						cnt++;
						ptr = strtok(NULL, delim);
					}
					cnt--;
				}

				if (cnt != 2) {
					fprintf(stderr, "Command form: unsubscribe <topic>\n");
					continue;
				}

				if (valid_topic_size == false) {
					fprintf(stderr, "Topic size should be <= %d\n", TOPIC_SIZE);
					continue;
				}

				valid_message = true;
			}

			if (valid_message) {
				// se trimite mesaj la server
				msg.len = 2 + sizeof(struct message_to_server);
				n = send(sockfd, &msg, msg.len, 0);
				DIE(n < 0, "send");

				if (msg.code == SUBSCRIBE_CODE) {
					printf("subscribed ");
				} else {
					printf("unsubscribed ");
				}

				printf("%s\n", content->topic);
			} else {
				// comanda invalida
				fprintf(stderr, "Commands expected: subscribe/unsubscribe/exit\n");
			}
		} else if (FD_ISSET(sockfd, &tmp_fds)) {
			// se primeste mesaj de la server
			memset(buffer, 0, BUFLEN);
			int N = recv(sockfd, buffer, BUFLEN - 1, 0);
			DIE(N < 0, "recv");

			
			if (N == 0) {
				// server-ul a intrerupt conexiunea
				break;
			}

			printf("%s", buffer);
		}
	}

	close(sockfd);

	return 0;
}
