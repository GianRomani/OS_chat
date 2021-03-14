#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  
#include <netinet/in.h> 
#include <sys/socket.h>
#include "linked_list.h"


ListHead* list_user; //lista utenti

//Per debug
void print_info_conn(struct sockaddr_in* client_addr){
    printf("INFO CONNESSIONE:\n");
    printf("%s\n",inet_ntoa(client_addr->sin_addr));
    //printf("%d\n",client_addr->sin_port);
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in sin = *client_addr;
    inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip)); 
    uint16_t port = htons(sin.sin_port);
    printf("%d\n",port);
}
//Per debug
void print_utenti_online(ListHead* head){
    printf("\nNumero di utenti online: %d\n",head->size);
    int i=0;
    ListItem* aux = head->first;
    printf("head->size: %d\n",head->size);
    while(aux){
        print_info_conn((struct sockaddr_in*)&aux->user->user_addr);
        //printListItem(aux);
        aux = aux->next;
        printf("\n");
    }
    printf("-----------------\n");
}

//Funzione che gestisce messaggi fra gli utenti
void gestisci_messaggio(struct Messaggio* newmsg, int sockfd, struct sockaddr* client_addr,socklen_t client_addr_len){
    char* dest = (char*)malloc(MAX_LEN_ID*sizeof(char));
    memset(dest,0,sizeof(dest));
    strcpy(dest,newmsg->dest);
    char* mitt = (char*)malloc(MAX_LEN_ID*sizeof(char));
    memset(mitt,0,sizeof(mitt));
    strcpy(mitt,newmsg->mitt); 
    int i;
    int ret=0;
    //controllo se client ha inviato messaggio 'QUIT' e in caso affermativo rimuovo l'utente dalla lista
    if (strncmp("server", dest, strlen(dest))==0){ 
        if(strncmp(quit_command, newmsg->msg, quit_command_len)==0){
            ret = sendto(sockfd,newmsg,sizeof(*newmsg),0, client_addr, client_addr_len); 
            printf("Ho inviato al client il messaggio di quit\n");
            stampaMsgDebug(newmsg);
            if(ret == -1){
                perror("Errore nella sendto!");
                exit(EXIT_FAILURE);
            }
            printf("Disconnessione utente in corso\n");
            ret=List_remove(list_user,mitt);
            if (ret==-1) {
                perror("Errore nella rimozione dell'utente!");
                exit(EXIT_FAILURE);
            }
        }
        else{
            //se il destinatario è server ma il messaggio non è quit
            pulisciMsg(newmsg);
            creaMsg(3, mitt, "server", "Ciao! Se vuoi uscire dalla chat inviami come messaggio: 'QUIT'\n", newmsg);
            ret = sendto(sockfd,newmsg,sizeof(*newmsg),0, client_addr, client_addr_len); 
            printf("Ho inviato al client un messaggio di saluto al client\n");
            stampaMsgDebug(newmsg);
            if(ret == -1){
                perror("Errore nella sendto!");
                exit(EXIT_FAILURE);
            }
        }
    }
    
    //Inoltra il messaggio se trova l'utente tra quelli online 
    else{
        ListItem* dest_found = List_find(list_user,dest);
        if(dest_found!=NULL){
            ret = sendto(sockfd,newmsg,sizeof(*newmsg),0, &dest_found->user->user_addr, client_addr_len);
            printf("Ho inviato\n");
            //print_info_conn((struct sockaddr_in*)&utenti_online[i].user_addr); //debug
            stampaMsgDebug(newmsg);
            if(ret == -1){
                perror("Errore nella sendto!");
                exit(EXIT_FAILURE);
            }
            //Spunta singola => invio ACK al mittente per avvertire che ho trovato il destinatario del suo messaggio
            pulisciMsg(newmsg);
            creaMsg(3, mitt, "server", "✔", newmsg); 
            ret = sendto(sockfd,newmsg,sizeof(*newmsg),0,client_addr, client_addr_len);
            stampaMsgDebug(newmsg);
            if(ret == -1){
                perror("Errore nella sendto!");
                exit(EXIT_FAILURE);
            }
        } 
        else{ //Non ho inviato nessun messaggio a un altro utente => destinatario non è online
            pulisciMsg(newmsg);
            creaMsg(3, mitt, "server", "L'utente selezionato non è online!\n", newmsg); 
            ret = sendto(sockfd,newmsg,sizeof(*newmsg),0,client_addr, client_addr_len);
            printf("Non ho inviato, utente inattivo\n");
            stampaMsgDebug(newmsg);
            if(ret == -1){
                perror("Errore nella sendto!");
                exit(EXIT_FAILURE);
            }
        }
    }
}
//Doppia spunta => ACK di ricezione del messaggio da parte del destinatario
void gestisci_ack(struct Messaggio* newmsg, int sockfd, struct sockaddr* client_addr,socklen_t client_addr_len){
    int ret; 
    printf("Ho ricevuto un ack da inoltrare\n");
    stampaMsgDebug(newmsg);
    ListItem* dest_found = List_find(list_user,newmsg->dest);
    if (dest_found!=NULL){
        ret = sendto(sockfd,newmsg,sizeof(*newmsg),0, (struct sockaddr*) &dest_found->user->user_addr, client_addr_len);
        if(ret == -1){
            perror("Errore nella sendto!");
            exit(EXIT_FAILURE);
        }
    }
}

void gestisci_registrazione(struct Messaggio* newmsg, int sockfd, const struct sockaddr* client_addr, socklen_t client_addr_len){
    int ret; 
    char buffer[MAX_MESSAGE];
    printf("E' stata richiesta una registrazione\n");
    int controllo = 0; //viene usato per decidere se scrivere sul file il nuovo utente
    //apro il file con gli utenti registrati
    FILE* file = fopen("utenti.txt","r+");
    if(file==NULL){
        perror("Errore nell'apertura del file");
    }
    //prendiamo ID utente per controllare se esiste
    char* newutente = (char*)malloc(MAX_LEN_ID*sizeof(char));
    strcpy(newutente,newmsg->msg);
    strtok(newutente,";");
    int size_newutente = sizeof(newutente);
    while(1){
        char* res = fgets(buffer,MAX_LEN_ID_AND_PASS,file);
        if(res == NULL) break;
        char* utente = strtok(buffer,";");
        int size_utente = sizeof(utente);
        int size = (size_newutente > size_utente)? size_newutente : size_utente;
        if(strncmp(newutente,utente,size) == 0){ //Se 0 allora l'ID esiste già
            //Inviamo messaggio d'errore di tipo al client (0 = registrazione)
            pulisciMsg(newmsg);
            creaMsg(0, "client", "server", "Il nome utente è già in uso!\n", newmsg); 
            printf("Sto inviando il seguente messaggio:\n");
            stampaMsgDebug(newmsg);
            ret = sendto(sockfd,newmsg,sizeof(*newmsg),0, client_addr, client_addr_len);
            //stampaMsgDebug(newmsg);
            if(ret == -1){
                perror("Errore nella sendto!");
                exit(EXIT_FAILURE);
            }
            controllo = -1;
            break;
        }
    }
    if(controllo != -1){ //se l'utente non è già registrato 
        ret = fprintf(file,newmsg->msg,sizeof(newmsg->msg));
        if(ret == -1){
            perror("Errore nella scrittura su file");
            exit(EXIT_FAILURE);
        }
        pulisciMsg(newmsg);
        creaMsg(3, "client", "server", "Registrazione effettuata con successo!\n", newmsg); 
        printf("Sto inviando il seguente messaggio:\n");
        stampaMsgDebug(newmsg);
        ret = sendto(sockfd,newmsg,sizeof(*newmsg),0,client_addr, client_addr_len);
        //stampaMsgDebug(newmsg);
        if(ret == -1){
            perror("Errore nella sendto!");
            exit(EXIT_FAILURE);
        }
        //l'utente risulta essere online dopo la registrazione
        Utente* user = (Utente*)malloc(sizeof(Utente));
        user->ID = newutente;
        user->user_addr = *client_addr;
        List_insert(list_user,user);
        print_utenti_online(list_user);
    }
    //Chiudo il file
    if(fclose(file) == -1) {
        perror("Errore nella chiusura del file");
        exit(EXIT_FAILURE);
    }
}

void gestisci_login(struct Messaggio* newmsg,int sockfd, struct sockaddr* client_addr,socklen_t client_addr_len){
    printf("E' stata richiesto un login \n");
    char buffer[MAX_MESSAGE];
    int buf_len = (sizeof(buffer));
    int ret; 
    int controllo = 0; //Viene modificato se l'utente effettua il login correttamente
    FILE* file = fopen("utenti.txt","r+");
    if(file==NULL){
        perror("Errore nell'apertura del file");
        exit(EXIT_FAILURE);
    }
    char* newutente = (char*)malloc(MAX_LEN_ID_AND_PASS*sizeof(char)); 
    strcpy(newutente,newmsg->msg);
    int size_newutente = sizeof(newutente);
    while(1){
        char* res = fgets(buffer,MAX_LEN_ID_AND_PASS,file);
        if(res == NULL) break;
        if(strncmp(newutente,buffer,buf_len) == 0){
            char* utente = strtok(newutente,";");
            int i;
            pulisciMsg(newmsg);
            if(List_find(list_user,utente)!=NULL){ //se NULL l'utente non è connesso
                creaMsg(1, "client", "server", "L'utente risulta essere già online!\n", newmsg);
            }
            //Inviamo messaggio di login effettuato al client
            if(newmsg->type==-1) creaMsg(3, "client", "server", "\nLogin effettuato con successo!\n", newmsg);
            printf("Sto inviando il seguente messaggio:\n");
            stampaMsgDebug(newmsg);
            ret = sendto(sockfd,newmsg,sizeof(*newmsg),0, client_addr, client_addr_len );
            if(ret == -1){
                perror("Errore nella sendto!");
                exit(EXIT_FAILURE);
            }
            //Aggiungiamo utente alla lista degli utenti online
            Utente* user = (Utente*)malloc(sizeof(Utente));
            user->ID = utente;
            user->user_addr = *client_addr;
            List_insert(list_user,user);

            print_utenti_online(list_user);
            controllo = -1;
            break;
        }
    }
    if(fclose(file) == -1) {
        perror("Errore nella chiusura del file");
        exit(EXIT_FAILURE);
    }
    if(controllo!=-1){ //Utente non registrato o errore nelle credenziali inserite
        pulisciMsg(newmsg);
        creaMsg(1, "client", "server", "Il nome utente o la password inseriti non sono corretti!\n", newmsg);
        printf("Sto inviando il seguente messaggio:\n");
        stampaMsgDebug(newmsg);
        ret = sendto(sockfd,newmsg,sizeof(*newmsg),0, client_addr, client_addr_len ); 
        if(ret == -1){
            perror("Errore nella sendto!");
            exit(EXIT_FAILURE);
        }
    }
    //stampaLista(list_user);
}

int main(){
    
    int sockfd, ret;
    char buffer[MAX_MESSAGE];
    size_t buf_len = sizeof(buffer);
    struct sockaddr_in server_addr, client_addr;
    int serv_addr_len = sizeof(server_addr);
    int client_addr_len = sizeof(client_addr);

    //socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM,0);
    if(sockfd < 0){
        perror("Errore nel creare la socket!");
        exit(EXIT_FAILURE);
    }

    //Pulisco le strutture per server e client
    memset(&server_addr,0,serv_addr_len);
    memset(&client_addr,0,client_addr_len);

    printf("Inserisci l'indirizzo IPv4 privato del server:\n");
    if (fgets(buffer, MAX_LEN_ID, stdin) != (char*)buffer) {
        perror("Errore leggendo da stdin!\n");
        exit(EXIT_FAILURE);
    }
    buffer[strlen(buffer)-1] = '\0';

    //Inizializzo server_addr
    server_addr.sin_family = AF_INET; //IPv4
    server_addr.sin_addr.s_addr = inet_addr(buffer);
    server_addr.sin_port = htons(13478);
    
    //Bind fra socket e indirizzo del server
    ret = bind(sockfd, (const struct sockaddr *)&server_addr, serv_addr_len);
    if(ret < 0){
        perror("Errore nella bind!");
        exit(EXIT_FAILURE);
    }
    struct Messaggio* newmsg = (struct Messaggio*)malloc(sizeof(struct Messaggio));
    //Comincio a comunicare con i client
    
    list_user = (ListHead*)malloc(sizeof(ListHead));
    ListHead_init(list_user);
    
    while(1){
        pulisciMsg(newmsg); 
        printf("Aspetto un messaggio da un client...\n");
        //Ricevo messaggio da un client
        ret = recvfrom(sockfd, newmsg, sizeof(*newmsg), 0, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
        if(ret == -1){
            perror("Errore nella recvfrom!");
        }
        printf("Messaggio ricevuto:\n");
        stampaMsgDebug(newmsg);
        int type = newmsg->type;
        
        if(type == 0){ 
            gestisci_registrazione(newmsg,sockfd,(struct sockaddr*)&client_addr, (socklen_t)client_addr_len);
        }
        else if(type == 1){ 
            gestisci_login(newmsg,sockfd,(struct sockaddr*)&client_addr, (socklen_t)client_addr_len);
        }
        else if(type == 2){
            gestisci_messaggio(newmsg,sockfd,(struct sockaddr*)&client_addr, (socklen_t)client_addr_len);
        }
        else if(type == 3){
            gestisci_ack(newmsg,sockfd,(struct sockaddr*)&client_addr, (socklen_t)client_addr_len);
        }
    }
     //Chiusura del server
    List_remove_all(list_user);
    if(close(sockfd) == -1) {
        perror("Errore nella chiusura del server!");
        exit(EXIT_FAILURE);
    }
    return 0;
}