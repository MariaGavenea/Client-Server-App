Gavenea Maria-Laurentia
321CA

					README

Programul contine urmatoarele fisiere:
- subscriber.cpp
- server.cpp
- helpers.h (contine macro-uri si structuri folosite in cele doua fisiere sursa)
- Makefile
- readme.txt

Pentru rezolvarea acestei teme am pornit de la scheletul laboratorului 8.

1. Clientii TCP (Subscriberii)

Acestia creeaza un socket TCP si se conecteaza la server prin acel socket.
Imediat dupa conectare, trimit serverului un mesaj TCP cu ID-ul lor (inainte sa faca
acest lucru, verifica daca id-ul primit ca parametru are cel mult 10 caractere).
Adauga in multimea de citire (read_fds) socket-ul TCP si file descriptorul pentru
date citite de la tastatura. Astfel, folosind multiplexarea (functia select()), clientii
pot citi date de la tastatura si sa comunice cu serverul.

a) Datele citite de la tastatura:

Sunt acceptate trei comenzi ca fiind valide:

subscribe <topic> <SF>
unsubscribe <topic>
exit

Daca comenzile nu au acest format, se afiseaza un mesaj de eroare sugestiv.
Pentru un topic cu lungime mai mare de 50 de caractere, si in cazul comenzii subscribe,
daca SF-ul nu are valoarea 0 sau 1, se afiseaza, de asemenea, un mesaj de eroare sugestiv.

b) Comunicarea cu serverul:

* Client -> Server
Clientii TCP pot trimite trei tipuri de mesaje serverului, identificate prin coduri:
- CLIENT_ID_CODE - trimite ID-ul (acest mesaj se trimite imediat dupa conectare, o singura data)
- SUBSCRIBE_CODE - trimite mesajul corespunzator unui subscribe
- UNSUBSCRIBE_CODE - trimite mesajul corespunzator unui unsubscribe
Serverul stie ca de la clientii TCP poate primi doar aceste tipuri de mesaje.

Structura unui astfel de mesaj este urmatoarea:
struct message {
	uint8_t len; - lungimea mesajului
	uint8_t code; - cod
	char payload[PAYLOAD_LEN]; - continutul (ID sau mesaj de subscribe/unsubscribe)
};

Pentru mesajele de subscribe/unsubscribe am mai definit o structura:
struct message_to_server {
	char topic[TOPIC_SIZE + 1]; - numele topicului
	bool SF; - SF
};

Folosindu-se de aceste doua structuri, serverul stie sa interpreteze mesajele de la
clientii TCP.

*Server -> Client
Clientii primesc mesaje de la server pe care doar le afiseaza. Clientii nu se ocupa de
aranjarea mesajelor, ci doar serverul. Astfel, am reusit sa tratez problema unirii mesajelor
atunci cand sunt primite de clientii TCP.

c) Inchiderea conexiunii - la primirea comenzii exit de la tastatura, clientul inchide conexiunea


2. Serverul

Acesta creeaza doi socketi, unul TCP si unul UDP, face bind pentru amandoi si asculta pe socketul TCP.
Adauga file descriptorii (socketul pe care se asculta conexiuni tcp, cel pe care se primesc mesaje udp si
cel pentru date citite de la tastatura) in multimea read_fds. Astfel, prin multiplexare, server-ul va putea
sa accepte conexiuni de la clientii TCP, sa comunice cu clientii TCP, UDP si sa citeasca date de la tastatura.

 a) Comunicarea cu clientii UDP

 Acesta primeste mesaje de la clientii UDP care respecta urmatorul format:

 struct message_udp {
	char topic[TOPIC_SIZE]; - nume topic
	uint8_t data_type; - tip de date
	char data[DATA_SIZE + 1]; - continut
};

In functie de tipul de date, el stie sa parseze continutul (cum este explicat in cerinta). Daca un mesaj
primit nu este conform cu formatul asteptat, serverul va afisa un mesaj de eroare si va ignora acel mesaj.
Daca mesajul primit este valid, il va trimite tuturor clientilor abonati la acel topic care sunt online, iar
pentru cei offline, care au SF = 1 pentru acel topic, il retine intr-o lista de mesaje asociate acelor clienti
(un map - messages_for_clients - care asociaza fiecare client cu o lista).

*Serverul aranjeaza mesajele primite de la clientii UDP astfel incat sa aiba formatul in care trebuiesc afisate
de catre clientii TCP.

b) Comunicarea cu clientii TCP

Serverul accepta conexiuni de la clientii TCP. Acesta prelucreaza doar mesajele de la clientii care i-au
trimis ID-ul. In cazul in care un client ii trimite un ID care exista deja (exista deja un client online cu
acel ID) sau un ID care are mai mult de 10 caractere, ii trimite clientului un mesaj de eroare corespunzator
si inchide conexiunea cu acel client.

Daca ID-ul este in regula, il retine intr-o structura (un map - socket_to_id) care asociaza socketii cu ID-uri. 
In map-ul clients_info face asocierea intre ID si informatii despre client : status (ONLINE/OFFLINE) si socket.
Daca ID-ul exista deja in map-ul clients_info, dar are statusul asociat offline, inseamna ca acel client tocmai
s-a reconectat. Verifica daca exista mesaje de transmis pentru acest client in map-ul messages_for_clients, iar
in cazul in care exista, trimite mesajele clientului si le sterge din structura.

Atunci cand un client se deconecteaza, ii schimba statusul in offline si il scoate din map-ul socket_to_id, si
inchide conexiunea pe acel socket.
La conectare/deconectare adauga/scoate socketii din multimea de descriptori (read_fds).

De la clientii TCP poate primi doar trei tipuri de mesaje, de lungime fixa (MESSAGE_MAX_LEN = 52). Astfel, am
rezolvat problema unirii mesajelor la primirea mesajelor de la clienti.

* Atunci cand primeste:

- mesaj de subscribe

Adauga intr-un map (topics_clients - face asocierea intre topic si lista de perechi <ID client, SF>, pentru
clientii abonati la acel topic) o pereche noua <ID, SF> doar daca ID-ul nu se regaseste deja in lista. Daca
este gasit, se modifica doar valoarea SF.

- mesaj de unsubscribe

Sterge din lista asociata acelui topic perechea corespunzatoare clientului care s-a dezabonat. Daca un client
incearca sa se dezaboneze de la un topic la care nu a fost abonat inainte, mesajul este ignorat.

c) Inchiderea conexiunii - la primirea comenzii exit de la tastatura, serverul inchide conexiunile (de la 
tastatura poate primi doar comanda exit, iar in caz contrar, afiseaza un mesaj de eroare). 


*Nota: Am dezactivat algoritmul Nagle folosind functia setsockopt(), cu optiunea TCP_NODELAY.
