 /*
 * chatterbox Progetto del corso di LSO 2017 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

#ifndef USER_H_
#define USER_H_

#include <string.h>
#include <config.h>
#include <pthread.h>
#include <icl_hash.h>
#include <message.h>
#include <list.h>

typedef struct {
	char name[MAX_NAME_LENGTH+1];
    list_t *history; // TODO REPLACE WITH LINKED LIST
    int connected;
    int fd;
} user_t;
    
// Registra un utente
int register_user(icl_hash_t *hash, pthread_mutex_t *mtx, char *name);

// Connette un utente
int connect_user(icl_hash_t *hash, pthread_mutex_t *mtx, char *name, unsigned long fd);

// Deregistra un utente
int unregister_user(icl_hash_t *hash, pthread_mutex_t *mtx, char *name);

// Disconnette un utente
int disconnect_user(icl_hash_t *hash, pthread_mutex_t *mtx, char *name);

// Restituisce la lista degli utenti come stringa
int user_list(icl_hash_t *hash, pthread_mutex_t *mtx, char **list);

// Post MSG
int post_msg(icl_hash_t *hash, pthread_mutex_t *mtx, char *name, user_msg_t msg);

// Post MSG ALL
int post_msg_all(icl_hash_t *hash, pthread_mutex_t *mtx, user_msg_t msg);

// retrieve message list
int retrieve_user_msg(icl_hash_t *hash, pthread_mutex_t *mtx, char *name, message_t **msg_list);

// is user connected
int connected_user(icl_hash_t *hash, pthread_mutex_t *mtx, char *name);

#endif /* USER_H_ */
