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
 * @file user.h
 * @brief Definizione delle strutture e funzioni per
 *        la gestione degli utenti sul server
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
  char name[MAX_NAME_LENGTH + 1];
  list_t *history;
  // list_t *groups;
  unsigned long user_fd;
} user_t;

typedef struct {
  pthread_mutex_t *mtx;
  icl_hash_t *hash;
  icl_hash_t *fd;
  unsigned long history_size;
  unsigned long connected;
} user_manager_t;

// Creo l'user manager
user_manager_t *create_user_manager(unsigned long history_size,
                                    unsigned long nbuckets);

// Distrugge l'user manager
void destroy_user_manager(user_manager_t *user_manager);

// Registra un utente
int register_user(user_manager_t *user_manager, char *name);

// Connette un utente
int connect_user(user_manager_t *user_manager, char *name, unsigned long fd);

// Deregistra un utente
int unregister_user(user_manager_t *user_manager, char *name);

// Disconnette un utente
int disconnect_user(user_manager_t *user_manager, char *name);

// Disconnette un utente dato fd
int disconnect_fd_user(user_manager_t *user_manager, unsigned long fd);

// Restituisce la lista degli utenti come stringa
int user_list(user_manager_t *user_manager, char **list);

// Lista fd degli online
int fd_list(user_manager_t *user_manager, int **list);

// Post MSG
int post_msg(user_manager_t *user_manager, char *name, message_t *msg);

// Post MSG ALL
int post_msg_all(user_manager_t *user_manager, message_t *msg);

// retrieve message list
list_t *retrieve_user_msg(user_manager_t *user_manager, char *name);

// is user connected
int connected_user(user_manager_t *user_manager, char *name);

void free_msg(void *msg);

// libera in memoria la struttura
void free_user(void *user);

// libera in memoria la struttura
void free_name_user(void *user);

#endif /* USER_H_ */
