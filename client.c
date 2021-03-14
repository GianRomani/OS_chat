#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  
#include <netinet/in.h> 
#include <sys/socket.h>
#include <signal.h>
#include "utils.h"

struct Messaggio* messaggio;
//Nome utente collegato
char* myUtente;

//Prendo ID da input ed effettuo controllo validità
int validazioneID(char* buf){
    int c=';'; 
    while(1){
        printf("Inserire ID:\n");
        memset(buf,0,sizeof(buf));
        if (fgets(buf, 2*MAX_LEN_ID, stdin) != (char*)buf) {
            perror("Errore leggendo da stdin!\n");
            exit(EXIT_FAILURE);
        } 
        int len = strlen(buf);
        if(len<5 || len>16) { //5 e 16 perchè viene contato '\n'
            printf("Lunghezza non valida (4/15 caratteri)\n");
            continue;
        }
        else if(strncmp(buf,"server",strlen("server"))==0) {
            printf("Id non valido\n");
            continue;
        }
        else if (strchr(buf,c)) {
            printf("Non puoi inserire ';'\n");
            continue;
        } 
        else return 1;
    }
}

//Prendo la password da input ed effettuo controllo validità
int validazionePassword(char* buf){
    int capital = 0;
    int digit = 0;
    char* s=(char*)malloc(1024*sizeof(char));
    while(1){
        printf("Inserire password:\n");
        memset(buf,0,sizeof(buf));
        if (fgets(buf, 2*MAX_LEN_ID, stdin) != (char*)buf) {
            perror("Errore leggendo da stdin!\n");
            exit(EXIT_FAILURE);
        }       
        int len = strlen(buf);
        if(len<9 || len>16)  { //9 e 16 perchè viene contato '\n'
            printf("Lunghezza non valida (8/15 caratteri)\n");
            continue;
        }
        s=buf;
        while (*s && !(capital && digit)){
            capital = (*s>='A' && *s<='Z' ? 1 : capital);
            digit = (*s>='0' && *s<='9' ? 1 : digit);
            s++;
        }
        if(*s) {
            buf[len]='\0';
            return 1;
        }
        else{
            printf("Password non sicura! Inserire almeno una cifra e una lettera maiuscola\n");
        }
    }
}

char* registrazione(int sockfd,struct sockaddr* server_addr, socklen_t* serv_addr_len){
    char buffer[MAX_MESSAGE];
    char buf_credenziali[MAX_LEN_ID_AND_PASS];
    int ret = 0;
    while(1){
        memset(buf_credenziali,0,sizeof(buf_credenziali));
        memset(buffer,0,sizeof(buffer));
        printf("Per registarti devi scegliere un ID (tra 4 e 15 caratteri) e una password (tra 8 e 15 caratteri, di cui almeno uno maiuscolo, e con almeno un numero).\n");
        //Prendo ID
        validazioneID(buffer);
        //Copio in buf_credenziali l' ID inserito dall'utente in buffer, che verrà poi azzerato
        strncpy(buf_credenziali,buffer,strlen(buffer)-1);
        //Memorizzo il nome utente in ris per poterlo ritornare e settare il nome utente online
        char* ris = (char*)malloc(MAX_LEN_ID*sizeof(char));
        strncpy(ris,buffer,strlen(buffer)-1);
        strcat(buf_credenziali,";"); // ID e password sono separate da un ';'
        memset(buffer,0,sizeof(buffer));
        //Prendo la password
        validazionePassword(buffer);
        //Aggiungo la password a buf_credenziali
        strcat(buf_credenziali,buffer);
        pulisciMsg(messaggio);
        creaMsg(0,"server","client",buf_credenziali,messaggio);
        //stampaMsgDebug(messaggio); //debug
        //Inviamo ID e password al server
        ret = sendto(sockfd, messaggio, sizeof(*messaggio), 0, server_addr, *serv_addr_len);
        if(ret == -1){
            perror("Errore nella sendto!");
            exit(EXIT_FAILURE);
        }
        //Ricezione messaggio di corretta o non corretta registrazione dal server
        pulisciMsg(messaggio);
        ret = recvfrom(sockfd, messaggio, sizeof(*messaggio), 0, server_addr, serv_addr_len);
        //stampaMsgDebug(messaggio); //debug
        if(ret == -1){
            perror("Errore nella recvfrom!");
            exit(EXIT_FAILURE);
        }
        printf("%s\n",messaggio->msg);
        if(messaggio->type == 3){
            printf("Ora puoi iniziare a chattare!\n");
            return ris;
        }
    }
}

char* login(int sockfd,struct sockaddr* server_addr, socklen_t* serv_addr_len){
    char buffer[MAX_MESSAGE];
    char buf_credenziali[MAX_MESSAGE];
    int ret = 0;
    //Memorizzo il nome utente in ris per poterlo ritornare e settare il nome utente online
    char* ris = (char*)malloc(MAX_LEN_ID*sizeof(char)); 
    
    while(1){
        printf("Per fare il login inserisci l'ID  e la password:\n");
        printf("Inserisci ID:\n");
        memset(buffer,0,sizeof(buffer));
        memset(buf_credenziali,0,sizeof(buf_credenziali));
        memset(ris,0,sizeof(ris));
        if (fgets(buffer, 2*MAX_LEN_ID, stdin) != (char*)buffer) {
            perror("Errore leggendo da stdin!\n");
            exit(EXIT_FAILURE);
        }
        //Salviamo il contenuto di buffer in buf_credenziali e cancelliamo in contenuto di buffer così da poterlo riusare per prendere l'input dell'utente
        strncpy(buf_credenziali,buffer,strlen(buffer)-1);
        strncpy(ris,buffer,strlen(buffer)-1);
        memset(buffer,0,sizeof(buffer));
        strcat(buf_credenziali,";"); //ID e password sono separate da un ';'
        printf("Inserisci password:\n");
        if (fgets(buffer, 2*MAX_LEN_ID, stdin) != (char*)buffer) {
            perror("Errore leggendo da stdin!\n");
            exit(EXIT_FAILURE);
        }
        //Concateniamo buffer e buf_credenziali
        strcat(buf_credenziali,buffer);
        //Invio messaggio al server
        pulisciMsg(messaggio);
        creaMsg(1,"server","client",buf_credenziali,messaggio);
        //stampaMsgDebug(messaggio); //debug
        ret = sendto(sockfd,messaggio,sizeof(*messaggio),0,server_addr,*serv_addr_len);
        if(ret==-1){
            perror("Errore nella sendto!");
            exit(EXIT_FAILURE);
        }
        pulisciMsg(messaggio);
        //Ricevo la risposta dal server
        ret=recvfrom(sockfd,messaggio,sizeof(*messaggio),0,server_addr,serv_addr_len);
        if(ret == -1){
            perror("Errore nella recvfrom");
            exit(EXIT_FAILURE);
        }
        //stampaMsgDebug(messaggio); //debug
        printf("%s",messaggio->msg);
        if(messaggio->type == 3){
            return ris; 
        }
    }
}

void chat(int sockfd, struct sockaddr* server_addr, socklen_t* serv_addr_len){
    char buf_user[MAX_LEN_ID];
    char buf_msg[MAX_MESSAGE];
    int ret;

    printf("Ciao %s, scegli a chi scrivere e poi inserisci il messaggio che vuoi inviare\n",myUtente);
    printf("Per uscire dalla chat invia il messaggio 'QUIT' inserendo come destinatario il server\n");
    pid_t pid = fork();

    if (pid>0){  //Il padre gestisce le ricezioni di messaggi da parte di altri utenti 
        while(1){
            pulisciMsg(messaggio);
            ret = recvfrom(sockfd, messaggio, sizeof(*messaggio), 0, server_addr, serv_addr_len);
            if(ret == -1){
                perror("Errore nella recvfrom!");
                exit(EXIT_FAILURE);
            } 
            if(messaggio->type!=3) stampaChatRicevuta(messaggio);
            else stampaAckRicevuto(messaggio);
            //Controlla se il messaggio ricevuto è QUIT
            if (strlen(messaggio->msg) == quit_command_len+1 && !memcmp(messaggio->msg, quit_command, quit_command_len)) {
                printf("Bye Bye!\n");
                kill(pid,SIGKILL); //uccidiamo il processo figlio per evitare errori con la fgets
                break;
            }
            if(messaggio->type!=3){
                char* mitt = (char*)malloc(sizeof(char));
                strcpy(mitt,messaggio->mitt);
                pulisciMsg(messaggio);
                creaMsg(3, mitt, myUtente, "✔✔", messaggio);
                stampaAckRicevuto(messaggio);
                //stampaChatInviata(messaggio); //debug
                //Invio messaggio
                ret = sendto(sockfd, messaggio, sizeof(*messaggio), 0, server_addr, *serv_addr_len);
                if(ret == -1){
                    perror("Errore nella sendto!");
                    exit(EXIT_FAILURE);
                }
            }
            //Per evitare la stampa tra i messaggi
            //stampaMsgDebug(messaggio); //debug
            if(messaggio->type != 3) printf("A quale utente vuoi scrivere?\n"); 
            else if(messaggio->type == 3 && strncmp(messaggio->msg,"✔",strlen(messaggio->msg)) != 0){
                printf("A quale utente vuoi scrivere?\n");
            }
        }
        exit(EXIT_SUCCESS);
    }
    else if(pid==0){ //il figlio gestisce gli invii dei messaggi agli utenti
        int status;
        while(1){
            memset(buf_user,0,sizeof(buf_user));
            memset(buf_msg,0,sizeof(buf_msg));
            pulisciMsg(messaggio);
            printf("A quale utente vuoi scrivere?\n");
            if (fgets(buf_user, 2*MAX_LEN_ID, stdin) != (char*)buf_user) {
                perror("Errore leggendo da stdin 1!\n");
                exit(EXIT_FAILURE);
            }
            buf_user[strlen(buf_user)-1]='\0';
            printf("Inserisci il tuo messaggio:\n");
            if (fgets(buf_msg, MAX_MESSAGE, stdin) != (char*)buf_msg) {
                perror("Errore leggendo da stdin!\n");
                exit(EXIT_FAILURE);
            }
            creaMsg(2, buf_user, myUtente, buf_msg, messaggio);
            stampaChatInviata(messaggio); 
            //Invio messaggio
            ret = sendto(sockfd, messaggio, sizeof(*messaggio), 0, server_addr, *serv_addr_len);
            if(ret == -1){
                perror("Errore nella sendto!");
                exit(EXIT_FAILURE);
            }
        }
    }
    else{
        perror("Errore nella fork!");
        exit(EXIT_FAILURE);
    } 
}

int main(){
    int sockfd, ret;
    struct sockaddr_in server_addr;
    int serv_addr_len = sizeof(server_addr);
    char buf_user[MAX_LEN_ID]; //Per scriverci input dell'utente

    myUtente = (char*)malloc(MAX_LEN_ID*sizeof(char));
    messaggio = (struct Messaggio*)malloc(sizeof(struct Messaggio));
    //socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM,0);
    if(sockfd < 0){
        perror("Errore nel creare la socket!");
        exit(EXIT_FAILURE);
    }
    
    printf("Inserisci l'indirizzo IPv4 del server:\n");
    if (fgets(buf_user, MAX_LEN_ID, stdin) != (char*)buf_user) {
        perror("Errore leggendo da stdin!\n");
        exit(EXIT_FAILURE);
    }
    buf_user[strlen(buf_user)-1] = '\0';

    //Pulisco la strutture server_addr
    memset(&server_addr,0,serv_addr_len);
    //Inizializzo server_addr
    server_addr.sin_family = AF_INET; //IPv4
    server_addr.sin_addr.s_addr = inet_addr(buf_user);
    server_addr.sin_port = htons(13478);
    memset(buf_user,0,MAX_LEN_ID);

    printf("Benvenuto in Private Chat!\n");
    while(1){
        printf("Se vuoi registrarti scrivi 'r', sei vuoi fare il login 'l':\n");
        if (fgets(buf_user, MAX_MESSAGE, stdin) != (char*)buf_user) {
            perror("Errore leggendo da stdin!\n");
            exit(EXIT_FAILURE);
        }
        if(!memcmp(buf_user, "l", 1)){
            myUtente = login(sockfd,(struct sockaddr*)&server_addr, (socklen_t*)& serv_addr_len);
            break;
        }
        else if(!memcmp(buf_user, "r", 1)){
            myUtente = registrazione(sockfd,(struct sockaddr*)&server_addr, (socklen_t*)& serv_addr_len);
            break;
        }
        else{
            printf("Risposta sbagliata, devi inserire 'r' o 'l'\n");
        }
        memset(buf_user,0,MAX_LEN_ID);
    }
    //Chatto con i miei amici
    chat(sockfd,(struct sockaddr*)&server_addr, (socklen_t*)& serv_addr_len);

    printf("Chiudo la connessione!\n");
    if(close(sockfd)==-1) {
        perror("Errore nella chiusura del client!"); 
        exit(EXIT_FAILURE);
    }
    return 0;
}
