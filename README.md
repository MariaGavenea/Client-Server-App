# Client-Server-App

Initializarea aplicatiei este data de pornirea server-ului, la care ulterior se vor putea conecta un numar variabil de clienti TCP si UDP.


## Clienti TCP (Subscribers)

Pornire:  ./subscriber **<ID_CLIENT> <IP_SERVER> <PORT_SERVER>**

- creeaza un socket TCP si se conecteaza la server prin acel socket
- trimit serverului un mesaj cu ID-ul lor
- pot citi date de la tastatura si sa comunice cu serverul

  ### 1. Datele citite de la tastatura

    Comenzi valide:
    - subscribe **<TOPIC\> <SF\>**
    - unsubscribe **<TOPIC\>**
    - exit

    Clientii TCP
    - se pot abona la/dezabona de la diverse topic-uri ale clientilor UDP
    - cat timp sunt conectati la server vor primi mesajele corespunzatoare topic-urilor la care s-au abonat
    - pentru fiecare topic pot opta pentru optiunea de store and forward **<SF\>** (SF = 0 - dezactivata, SF = 1 - activata) 
    - daca un client este deconectat si s-a abonat inainte la un topic cu optiunea de SF activata, cand se va reconecta va primi toate mesajele corespunzatoare acelui topic care au fost trimise server-ului de catre clientii UDP in timpul in care clientul TCP a fost deconectat

  ### 2. Comunicarea cu server-ul

    **a) Client -> Server**
    
    Clientii TCP pot trimite trei tipuri de mesaje serverului, identificate prin coduri:
    - CLIENT_ID_CODE - trimite ID-ul (acest mesaj se trimite imediat dupa conectare, o singura data)
    - SUBSCRIBE_CODE - trimite mesajul corespunzator unui subscribe
    - UNSUBSCRIBE_CODE - trimite mesajul corespunzator unui unsubscribe


    Serverul stie ca de la clientii TCP poate primi doar aceste tipuri de mesaje.

    **b) Server -> Client**
    
    Clientii primesc mesaje de la server pe care doar le afiseaza. Clientii nu se ocupa de aranjarea mesajelor, ci doar serverul. 
    
    
## Clienti UDP

Pornire: python3 udp_client.py **<IP_SERVER> <PORT_SERVER>**         

Mesajele formate de clientii UDP contin urmatoarele informatii:
- nume topic
- tip de date
- continut

Asocierea (Tip de date : Continut) 
- 0 (INT) : octet de semn urmat de un uint32t formatat conform network byte-order
- 1 (SHORT_REAL) : uint16_t reprezentand modulul numarului inmultit cu 100
- 2 (FLOAT) : un byte de semn + un uint32_t (in network byte-order) reprezentand modulul numarului obtinut din alipirea partii intregi de partea zecimala a numarului; un uint8_t ce reprezinta modulul puterii negative a lui 10 cu care trebuie inmultit modulul pentru a obtine numarul original (in modul)
- 3 (STRING) : sir de maxim 1500 de caractere


## Server

Pornire:  ./server **<PORT_DORIT>**

- asteapta conexiuni/datagrame pe portul **<PORT_DORIT>**
- la primirea comenzii exit de la tastatura, serverul inchide conexiunile
- comunica cu clientii TCP si UDP si retine diverse informatii de la acestia

  ### 1) Comunicarea cu clientii UDP

  Serverul
    - primeste mesaje de la clientii UDP si in functie de tipul de date, parseaza continutul 
    - daca un mesaj primit nu este conform cu formatul asteptat, va afisa un mesaj de eroare si va ignora acel mesaj
    - daca mesajul primit este valid, il va trimite tuturor clientilor abonati la acel topic care sunt online, iar pentru cei offline, care au SF = 1 pentru acel topic, il retine intr-o lista de mesaje asociate acelor clienti
  
   ### 2) Comunicarea cu clientii TCP
   - fiecare client este identificat prin ID-ul cu care acesta a fost pornit si serverul nu permite ca mai multi clienti cu acelasi ID sa fie conectati in acelasi timp
   - serverul retine informatii despre statusul clientilor (daca sunt online/offline) si le trimite celor online mesajele corespunzatoare topic-urilor la care au fost abonati
