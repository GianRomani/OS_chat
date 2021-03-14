#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h> 

#define quit_command "QUIT" 
#define quit_command_len  4 
#define MAX_LEN_ID 16
#define MAX_LEN_ID_AND_PASS 34 //16 ID + 16 password + ; di concatenazione + \n alla fine
#define MAX_MESSAGE 1024 //lunghezza massima del campo testuale

typedef struct Utente{
    char* ID;  //Id con cui l'utente si è registrato
    struct sockaddr user_addr; //info sulla socket a cui l'utente è connesso
}Utente;

typedef struct Messaggio{
    int type; //Tipo di messaggio: 0->registrazione; 1->login; 2->messaggio; 3->ack
    char dest[MAX_LEN_ID]; 
    char mitt[MAX_LEN_ID]; 
    char msg[MAX_MESSAGE];
}Messaggio;

//creasMsg viene utiizzato per riempire la struttura del messaggio
void creaMsg(int type, char* dest, char* mitt, char * msg, struct Messaggio* messaggio){ 
    //memset(messaggio,0,sizeof(*messaggio));
    messaggio->type = type;
    strcpy(messaggio->dest,dest);
    strcpy(messaggio->mitt,mitt);
    strcpy(messaggio->msg,msg);
}

//Stampa messaggi all'interno delle chat degli utenti
void stampaChatRicevuta(struct Messaggio* messaggio){
  printf("Da %s: %s\n",messaggio->mitt,messaggio->msg);
}
//Stampa messaggi all'interno delle chat degli utenti
void stampaChatInviata(struct Messaggio* messaggio){
  printf("A %s: %s\n",messaggio->dest,messaggio->msg);
}
//Stampa ack all'interno della chat degli utenti
void stampaAckRicevuto(struct Messaggio* messaggio){
  printf("ACK da %s: %s\n\n",messaggio->mitt, messaggio->msg);
}

//Stampa messaggi per debug
void stampaMsgDebug(struct Messaggio* messaggio){
    printf("   MESSAGGIO\n");
    if(messaggio->msg!=NULL && messaggio->dest!=NULL && messaggio->mitt!=NULL) {
        printf("   type: %d, dest: %s, mitt: %s, msg: %s",messaggio->type,messaggio->dest,messaggio->mitt,messaggio->msg);
    }
    else if(messaggio->msg!=NULL) {
        printf("   type: %d, msg: %s",messaggio->type,messaggio->msg);
    }
    else{
        printf("   Messaggio vuoto\n");
    }
    printf("   dim totale msg: %lu\n",sizeof(*messaggio));
    printf("   type dim: %lu, dest dim: %lu, mitt dim:%lu, msg dim: %lu\n\n",sizeof(messaggio->type),strlen(messaggio->dest),strlen(messaggio->mitt),strlen(messaggio->msg));
}

//Azzerra i campi di messaggio
void pulisciMsg(struct Messaggio* messaggio){
    messaggio->type = -1;
    memset(messaggio->dest,0, MAX_LEN_ID*sizeof(char));
    memset(messaggio->mitt,0, MAX_LEN_ID*sizeof(char));
    memset(messaggio->msg,0, MAX_MESSAGE*sizeof(char)); 
}