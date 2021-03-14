Private Chat project

Compilazione: 
Usare il Makefile oppure gcc -o server server.c linked_list.h e gcc -o client client.c utils.h

Struttura generale:
Nel realizzare questo progetto, abbiamo deciso di implementare come funzionalità la registrazione e il login degli utenti e lo scambo di messaggi tra gli utenti online.
Il protocollo usato per far comunicare server e client è UDP. Il server riceve e invia messaggi tramite una singola porta (13478). Per questo motivo abbiamo deciso di memorizzare le informazioni degli utenti connessi con una linked list (definite, con le funzioni relative, nel file linked_list.h) di strutture Utente, i cui campi sono ID ( nome con cui l'utente si è registrato ) e una struttura sockaddr user_addr, necessaria per avere le informazioni utili per comunicare con il singolo client.
Per avere memoria degli utenti registrati, gli ID e le password sono concatenati con ';' e salvati, una riga per utente, nel file utenti.txt.
La comunicazione tra server e client avviene tramite una struttura Messaggio che contiene i seguenti campi: 
il tipo di messaggio scambiato, il destinatario, il mittente e il campo testuale vero e proprio del messaggio. 
Il tipo di messaggio serve al server per offrire funzionalità diverse: 0->registrazione, 1->login, 2->messaggio, 3->ack.
Non essendoci comunicazione diretta tra singoli client la gestione del rilancio dei messaggi è affidata al server, che si occupa anche di inviare gli ack ai client per indicare l'avvenuta ricezione da parte prima del server e poi del destinatario finale. 
L'utente può chattare con qualsiasi utente online contemporaneamente senza dover aspettare ack o risposta dal destinatario.
Per disconnettersi l'utente invia il messaggio 'QUIT' al server.
In utils.h sono presenti le funzioni che creano, puliscono e stampano i messaggi.

Avvio:
Affinché client e server comunichino in reti diverse, abbiamo creato un forwarding dei pacchetti UDP ricevuti dal router al suo ip globale verso la porta 13478 del pc su cui facciamo girare il server (identificato dal suo ip locale). Per realizzarlo abbiamo creato un associazione di porte nel router della rete. 
Per questo motivo l'ip da inserire all'avvio del server è il suo ip locale nella rete, mentre quello da fornire ai client è l'ip globale del router.

Main:
Nel main del server vengono dichiarate e inizializzate le variabili utilizzate successivamente, viene aperta la connessione attraverso la socket e viene chiamata la bind per legare la socket all'ip del server. Quindi vi è un ciclo while con una recvfrom() e a seconda del tipo di messaggio ricevuto vengono chiamate le funzioni gestisci_registrazione(), gestisci_login(), gestisci_messaggio() e gestisci_ack().
Il main del client chiede all'utente se vuole effettuare il login o la registrazione, in un ciclo while da cui si esce quando l'utente è loggato, chiamando la funzione per la gestione della chat (chat()).

Registrazione (messaggi tipo 0):
Al client, subito dopo aver inserito l'ip del server a cui connettersi, viene chiesto se si vuole effettuare il login o la registrazione. Scrivendo 'r' si passerà alla registrazione di un nuovo utente, quindi sarà necessario scegliere un nuovo ID e una password che rispettino dei parametri ( che vengono controllati nelle funzioni validazioneID e validazionePassword ). ID e password vengono concatenati e inviati al server che si occuperà di controllare se l'ID è già presente nel file utenti.txt. Se il nome utente è già usato, il server invia indietro un messaggio di tipo 0, altrimenti di tipo 3. L'utente aspetta la risposta dal server e a seconda del tipo di messaggio che riceve passa alla chat oppure richiede nuovamente la registrazione. Se la registrazione è andata a buon fine il server inserisce l'utente nella lista degli utenti online (attraverso la List_insert()) e il client memorizza il proprio nome utente per uso futuro. 

Login (messaggi tipo 1):
Nel client si richiede un login inserendo 'l'. Vengono richieste le credenziali che vengono concatenate e inviate al server, che controlla se sono corrette scandendo il file utenti.txt. In caso positivo, l'utente viene inserito nella lista degli utenti online dal server, mentre, lato client, si passa alla chat. In caso negativo, il server invia un messaggio d'errore di tipo 1 e il client richiede ID e password all'utente.

Chat (messaggio tipo 2):
Nel client si effettua una fork() in modo tale da poter gestire contemporaneamente ricezione e invio di messaggi da più utenti. In particolare il padre gestisce le ricezioni di messaggi da parte di altri utenti, mentre il figlio gli invii.
In ricezione vengono stampati i messaggi, inviati gli ack di doppia spunta al server e controllato se è stato inviato al server il messaggio 'QUIT'. In questo caso il server elimina il client dalla lista degli utenti online e manda indietro il messaggio di saluto. Il padre, ricevendo questo messaggio, manda un segnale al processo figlio per terminarlo e poi termina anch'esso.
Nella gestione degli invii invece, il processo figlio crea il messaggio da inviare e lo manda al server che si occuperà di inoltrarlo al destinatario.
Per inoltrare il messaggio il server chiama la funzione List_find() per verificare se il destinatario è online, se non è così lo comunica al client, altrimenti inoltra il messaggio e invia l'ack con singola spunta al mittente per avvertirlo che sta inviando il suo messaggio al destinatario.

Ack (messaggio tipo 3):
Gli ack sono mandati dal server al client per notificare una registrazione o un login avvenuti in maniera corretta o per 
comunicare se il messaggio è arrivato al server (✔) e al destinatario (✔✔). Questo secondo caso è gestito dalla funzione gestisci_ack().