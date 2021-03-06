#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <math.h>
#include "helpers.h"
#include <string>
#include <list>
#include <map>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	std::map<int, std::string> socket_to_id;  // face asocierea intre socket si id
	std::map<std::string, client_info> clients_info;  // contine informatii despre status si socket
	// retine mesajele pentru clientii cu SF 1 care sunt offline
	std::map<std::string, std::list<std::string>> messages_for_clients;
	// fiecarui topic ii corespunde cate o lista de perechi <client ID, SF> (clienti abonati la el)
	std::map<std::string, std::list<std::pair<std::string, bool>>> topics_clients;

	int sockfd_tcp, sockfd_udp, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr, udp_cli_addr;
	int n, i, ret;
	socklen_t clilen, socklen = sizeof(struct sockaddr_in);

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// se creaza socket-ul tcp
	sockfd_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	DIE(sockfd_tcp < 0, "socket tcp");

	// TCP NO DELAY
	int flag_delay = 1;
	int tcp_no_delay = setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, &flag_delay, sizeof(int));

	// se creaza socket-ul udp
	sockfd_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	DIE(sockfd_udp < 0, "socket udp");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// bind pentru tcp
	ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp");

	// bind pentru udp
	ret = bind(sockfd_udp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind udp");

	ret = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga file descriptorii (socketul pe care se asculta conexiuni tcp, cel pe care se
	// primesc mesaje udp si cel pentru date citite de la tastatura) in multimea read_fds
	FD_SET(sockfd_tcp, &read_fds);
	FD_SET(sockfd_udp, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	fdmax = sockfd_tcp > sockfd_udp ? sockfd_tcp : sockfd_udp;

	bool stop = false;

	while (!stop) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd_tcp) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd_tcp, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					continue;
				}

				if (i == STDIN_FILENO) {
					// daca a primit exit de la tastatura
					// inchide tot
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					if (strncmp(buffer, "exit\n", 5) == 0) {
						stop = true;
						break;
					} else {
						fprintf(stderr, "Command expected: exit\n");
					}
					
					continue;
				}

				if (i == sockfd_udp) {
					// mesaj udp
					bool correct_data = true;
					struct message_udp msg_udp;

					memset(&msg_udp, 0, UDP_MESSAGE_LEN);
					n = recvfrom(i, &msg_udp, UDP_MESSAGE_LEN - 1, 0,
						(struct sockaddr *) &udp_cli_addr, &socklen);
					DIE(n < 0, "recvfrom");

					// construire mesaj pentru clientii tcp
					std::string data(inet_ntoa(udp_cli_addr.sin_addr));
					char port[PORT_LEN];

					data += ":";
					sprintf(port, "%d", ntohs(udp_cli_addr.sin_port));
					data += port;
					data += " - ";
					data += msg_udp.topic;
					data += " - ";

					switch (msg_udp.data_type) {
						case 0 : {
									data += "INT - ";

								 	long long value = (long long) (ntohl(*(uint32_t*)(msg_udp.data + 1)));
								 	uint8_t sign = msg_udp.data[0];

								 	if (sign == 1) {
								 		value *= (-1);
								 	} else {
								 		if (sign != 0) {
								 			correct_data = false;
								 			break;
								 		}
								 	}

								 	char val[11];
								 	sprintf(val, "%lld", value);
								 	data += val;
							     } break;

						case 1 : {
									data += "SHORT_REAL - ";
									double value = ntohs(*(uint16_t*)(msg_udp.data));
            						value /= 100; 

            						char val[15];
            						sprintf(val, "%.2f", value);
            						data += val;

							     } break;
						case 2 : {
									data += "FLOAT - ";
									double value = ntohl(*(uint32_t*)(msg_udp.data + 1));
									value /= pow(10, msg_udp.data[5]);
									uint8_t sign = msg_udp.data[0];

            						if (sign == 1) {
                						value *= (-1);
            						} else {
								 		if (sign != 0) {
								 			correct_data = false;
								 			break;
								 		}
								 	}

            						char val[25];
            						std::string format("%.");
            						char num[4];
            						sprintf(num, "%u", msg_udp.data[5]);
            						format += num;
            						format += "lf";

            						sprintf(val, format.c_str(), value);
            						data += val;
							     } break;
						case 3 : {
									data += "STRING - ";
									data += msg_udp.data;
								 } break;

						default: 	correct_data = false;
						 			break;
					}

					if (correct_data == false) {
						fprintf(stderr, "Incorrect data from udp client\n");
					}

					data += '\n';
					
					for (std::pair<std::string, bool> info : topics_clients[msg_udp.topic]) {
						if (clients_info[info.first].status == ONLINE) {
							// trimit mesajele la clientii abonati care sunt online
							n = send(clients_info[info.first].sock, data.c_str(), data.size(), 0);
							DIE(n < 0, "send");
						} else {
							if (info.second == 1) {
								// daca SF = 1 si clientul este offline
								// retin datele pentru el
								messages_for_clients[info.first].push_back(data);
							}
						}
					}

					continue;
				}

				// s-au primit date pe unul din socketii de client tcp,
				// asa ca serverul trebuie sa le receptioneze
				struct message msg;
				memset(&msg, 0, MESSAGE_MAX_LEN);

				n = recv(i, &msg, MESSAGE_MAX_LEN, 0);
				DIE(n < 0, "recv");

				if (n == 0) {
					std::string id(socket_to_id[i]);
					// conexiunea s-a inchis
					printf("Client %s disconnected.\n", id.c_str());
					clients_info[id].status = OFFLINE;
					socket_to_id.erase(i);

					close(i);
						
					// se scoate din multimea de citire socketul inchis 
					FD_CLR(i, &read_fds);
				} else {
					// ID
					if (msg.code == CLIENT_ID_CODE) {
						std::string id(msg.payload);

						if (id.size() > CLIENT_ID_LEN) {
							// daca lungime ID > 10
							// trimite un mesaj corespunzator inapoi clientului
							std::string m("Client's ID should have at most 10 characters\n");
							
							n = send(i, m.c_str(), m.size(), 0);
							DIE(n < 0, "send");

							// inchide conexiunea
							close(i);
							FD_CLR(i, &read_fds);
							continue;
						}

						// daca exista deja ID-ul in map
						if (clients_info.count(id) > 0) {
							if (clients_info[id].status == ONLINE) {
								// daca e deja cineva conectat cu acest ID
								// trimite un mesaj corespunzator inapoi clientului
								std::string m("Another client already has this ID.\n");

								n = send(i, m.c_str(), m.size(), 0);
								DIE(n < 0, "send");

								// inchide conexiunea
								close(i);
								FD_CLR(i, &read_fds);
							} else {
								// se reconecteaza vechiul client
								socket_to_id[i] = id;
								clients_info[id].status = ONLINE;
								clients_info[id].sock = i;
								printf("New client %s connected from %s:%d\n", id.c_str(),
									inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

								if (messages_for_clients.count(id) > 0) {
									// daca exista mesaje de transmis pentru acest client
									// trimite mesajele salvate pentru el

									for (std::string data : messages_for_clients[id]) {
										n = send(i, data.c_str(), data.size(), 0);
										DIE(n < 0, "send");
									}

									// sterge mesajele
									messages_for_clients[id].clear();
									messages_for_clients.erase(id);
								}
							}

							continue;
						}

						// client nou
						socket_to_id[i] = id;
						clients_info[id].status = ONLINE;
						clients_info[id].sock = i;

						printf("New client %s connected from %s:%d\n", id.c_str(),
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
						continue;
					}

					if (socket_to_id.count(i) > 0) {
						// exista asociere intre socket si ID (clientul a trimis ID-ul si e unul valid)
						struct message_to_server *content = (struct message_to_server *) msg.payload;
						std::string topic(content->topic);
						std::string id(socket_to_id[i]);

						// subscribe
						if (msg.code == SUBSCRIBE_CODE) {
							bool found = false;
							for (auto it = topics_clients[topic].begin();
								it != topics_clients[topic].end(); it++) {
								if (id.compare((*it).first) == 0) {
									// modifica SF pentru subscriber deja abonat la acest topic
									(*it).second = content->SF;
									found = true;
									break;
								}
							}

							if (found == false) {
								// adauga subscriber nou la acest topic
								std::pair<std::string, bool> p(id, content->SF);
								topics_clients[topic].push_back(p);
							}

							continue;
						}

						// unsubscribe
						if (msg.code == UNSUBSCRIBE_CODE) {
							if (topics_clients.count(topic) > 0) {
								// se poate dezabona de la un topic la care a fost abonat anterior
								// daca nu a fost abonat la acel topic, mesajul este ignorat
								int pos;
								for (auto it = topics_clients[topic].begin();
									it != topics_clients[topic].end(); it++) {
									if (id.compare((*it).first) == 0) {
										topics_clients[topic].erase(it);
										break;
									}
								}

							}
						}
					}
				}
			}
		}
	}

	close(sockfd_tcp);
	close(sockfd_udp);

	return 0;
}
