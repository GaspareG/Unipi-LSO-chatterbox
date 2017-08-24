/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 * Autore: Gaspare Ferraro CORSO B - Matricola 520549
 * Tale sorgente è, in ogni sua parte, opera originale di Gaspare Ferraro
 */
/**
 * @file group.h
 * @brief Dichiarazione delle strutture e funzioni necessarie
 *        alla gestione dei gruppi nel server
 */

#ifndef GROUP_H_
#define GROUP_H_

#include <config.h>
#include <user.h>

typedef struct  {
  char groupname[MAX_NAME_LENGTH+1];
  icl_hash_t *member; 
  int size;   
} group_t;

typedef struct {
  user_manager_t *user_manager;
  pthread_mutex_t *mtx;
  icl_hash_t *groups;  
  unsigned long buckets;
} group_manager_t ;

// Creazione del group manager
group_manager_t* create_group_manager(user_manager_t* usr_mng, unsigned long nbuckets);
  
// Distruzione del group manager
void destroy_group_manager(group_manager_t* manager);
  
// Creazione di un gruppo 
int create_group(group_manager_t* manager, char *groupname);

// Adesione di un utente ad un gruppo
int join_group(group_manager_t* manager, char *groupname, char *username);

// Abbandono di un utente di un gruppo
int leave_group(group_manager_t* manager, char *groupname, char *username);

// Controllo se utente in gruppo
int in_group(group_manager_t* manager, char *groupname, char *username);

// Controllo se utente il gruppo
int exists_group(group_manager_t* manager, char *groupname);

// Invio di un messaggio in gruppo
int members_group(group_manager_t* manager, char *groupname, char **list);

// Libera un gruppo in memoria
void free_group(void *group);
#endif /* GROUP_H_ */
