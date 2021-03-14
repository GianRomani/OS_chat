#include "utils.h"

//Struttura che contiene le informazioni sugli utenti attualmente connessi
typedef struct ListItem {
  struct Utente* user;
  struct ListItem* next;
} ListItem;

typedef struct ListHead {
  ListItem* first;
  ListItem* last;
  int size;
} ListHead;

//Inizializza ListItem
void ListHead_init(ListHead* head) {
  head->first = NULL;
  head->last = NULL;
  head->size = 0;
}

//Inizializza ListHead
void ListItem_init(ListItem* newUser, Utente* utente){
  newUser->next = NULL;
  newUser->user = utente;
}

//Per debug
void printListItem(ListItem* item){
  if(item==NULL) printf("NULL");
  printf("ListItem-> user->ID:%s\n",item->user->ID);
  char ip[INET_ADDRSTRLEN];
  struct sockaddr_in *sin_p = (struct sockaddr_in*) &item->user->user_addr;
  struct sockaddr_in sin = *sin_p;
  inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip)); 
  uint16_t port = htons(sin.sin_port);
  printf("ListItem-> user->ip:%s user->port:%d\n",ip,port); 
}
//Per debug
void stampaLista(ListHead* head){
  int i=0;
  ListItem* aux = head->first;
  printf("head->size: %d\n",head->size);
  while(aux){
    printListItem(aux);
    aux = aux->next;
  }
  printf("-----------------\n");
}

//Cerca nella lista l'ID dell'utente
ListItem* List_find(ListHead* head, char* utente) {
  ListItem* aux = head->first;
  while(aux){
    //printListItem(aux);
    if (strncmp(aux->user->ID,utente,strlen(utente)) == 0) return aux;
    aux = aux->next;
  }
  return NULL;
}

//Inserisce un nuovo utente all'inizio della lista
int List_insert(ListHead* head, Utente* utente) {
  //printf("Inserisco l'utente: %s\n",utente->ID);
  ListItem* user_found = List_find(head,utente->ID);
  if(user_found == NULL){ //L'utente non ha già effettuato il login 
    ListItem* newUser = (ListItem*)malloc(sizeof(ListItem));
    ListItem_init(newUser,utente);
    if(head->size == 0){ //newUser è il primo utente a fare il login (sarà l'ultimo della lista)
      head->first = newUser;
      head->last = newUser; 
    }
    else{ 
      newUser->next = head->first;
      head->first = newUser;
    }
    head->size++;
    return 1;
  }
  else return -1;
}

//Rimuove l'utente con ID utente dalla lista
int List_remove(ListHead* head, char* utente) {
  ListItem* aux = head->first;
  ListItem* next_user;
  if(head->size == 0) return -1;
  
  if (strncmp(aux->user->ID,utente,strlen(utente)) == 0){
    head->first = aux->next;
    head->size--;
    free(aux);
    return 1;
  }

  while(aux->next){
    next_user = aux->next;
    if (strncmp(next_user->user->ID,utente,strlen(utente)) == 0){
      aux->next = next_user->next;
      head->size--;
      free(next_user);
      return 1;
    }
    aux = aux->next;
  }
  //L'utente non era connesso
  return -1;
}

//Elimino tutti gli utenti dalla lista
void List_remove_all(ListHead* head){
  ListItem* aux = head->first;
  ListItem* act;
  while(aux){
    act = aux;
    aux = aux->next;
    free(act);
    head->size--;
  }
  head->first = NULL;
}
