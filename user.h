/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file user.h
 * @brief Definizione delle strutture e funzioni per
 *        la gestione degli utenti sul server
 *
 * @author Gaspare Ferraro 520549 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 * originale dell'autore  
 */

#ifndef USER_H_
#define USER_H_

#include <string.h>
#include <config.h>
#include <pthread.h>
#include <icl_hash.h>
#include <message.h>
#include <list.h>

/**
 *  @struct user
 *  @brief informazioni di un utente
 *
 *  @var name    nickname utente
 *  @var history lista dei messaggi da consegnare
 *  @var user_fd file descript dell'utente loggato
 */
typedef struct {
  char name[MAX_NAME_LENGTH + 1];
  list_t *history;
  unsigned long user_fd;
} user_t;

/**
 *  @struct user_manager
 *  @brief gestore degli utenti
 *
 *  @var mtx          mutex per la m.e.
 *  @var hash         hashmap <nickname, struttura user>
 *  @var fd           hashmap <fd, nickname>
 *  @var history_size massimo num messaggi da memorizzare per utente
 *  @var connected    numero degli utenti connessi
 */
typedef struct {
  pthread_mutex_t *mtx;
  icl_hash_t *hash;
  icl_hash_t *fd;
  unsigned long history_size;
  unsigned long connected;
} user_manager_t;

/**
 * @function create_user_manager
 * @brief crea la struttura di gestione degli utenti
 *
 * @return puntatore all'user_manager creato
 * @param history_size dimensione della cronologia per ogni utente
 * @param nbuckets     numero dei bucket per le hashmap
 */
user_manager_t *create_user_manager(unsigned long history_size,
                                    unsigned long nbuckets);

/**
 * @function destroy_user_manager
 * @brief libera la memoria occupata dall'user_manager
 *
 * @param user_manager puntatore all'user_manager da liberare
 */
void destroy_user_manager(user_manager_t *user_manager);

/**
 * @function register_user
 * @brief registra un nuovo utente
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname da registrare
 * @return  0 se utente registrato correttamente
 *          -1 per errore
 */
int register_user(user_manager_t *user_manager, char *name);

/**
 * @function connect_user
 * @brief connette un nuovo utente
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname da connettere
 * @param fd           descriptor attuale del client connesso
 * @return  0 se utente connesso correttamente
 *          -1 per errore
 */
int connect_user(user_manager_t *user_manager, char *name, unsigned long fd);

/**
 * @function unregister_user
 * @brief deregistra un utente registrato
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname da deregistrare
 * @return  0 se utente deregistrato correttamente
 *          -1 per errore
 */
int unregister_user(user_manager_t *user_manager, char *name);

/**
 * @function disconnect_user
 * @brief disconnette un utente registrato
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname da disconnettere
 * @return  0 se utente deregistrato correttamente
 *          -1 per errore
 */
int disconnect_user(user_manager_t *user_manager, char *name);

/**
 * @function disconnect_fd_user
 * @brief disconnette un utente registrato dato il suo fd
 *
 * @param user_manager puntatore all'user_manager
 * @param fd           fd dell'utente da disconnettere
 * @return  0 se utente deregistrato correttamente
 *          -1 per errore
 */
int disconnect_fd_user(user_manager_t *user_manager, unsigned long fd);

/**
 * @function user_list
 * @brief restituisce la lista degli utenti connessi attualmente
 *
 * @param user_manager puntatore all'user_manager
 * @param list         area dove allocare la stringa con la lista utenti
 * @return  n>=0 numero degli utenti attualmente collegati
 *          -1 per errore
 */
int user_list(user_manager_t *user_manager, char **list);

/**
 * @function fd_list
 * @brief restituisce la lista dei fd degli utenti connessi attualmente
 *
 * @param user_manager puntatore all'user_manager
 * @param list         area dove allocare la lista con i fd degli utenti
 * @return  n>=0 numero degli utenti attualmente collegati
 *          -1 per errore
 */
int fd_list(user_manager_t *user_manager, int **list);

/**
 * @function post_msg
 * @brief salva in memoria un messaggio da consegnare ad un utente 
 *        non connesso
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname dell'utente a cui salvare il messaggio
 * @param msg	       puntatore al messaggio da salvare
 * @return   0 operazione effettuata correttamente
 *          -1 per errore
 */
int post_msg(user_manager_t *user_manager, char *name, message_t *msg);

/**
 * @function post_msg_all
 * @brief salva in memoria un messaggio da consegnare ad ogni utente
 *        non connesso attualmente
 *
 * @param user_manager puntatore all'user_manager
 * @param msg	       puntatore al messaggio da salvare
 * @return   >= 0 numero dei messaggi salvati
 *          -1 per errore
 */
int post_msg_all(user_manager_t *user_manager, message_t *msg);

/**
 * @function retrieve_user_msg
 * @brief restituisce la lista dei messaggi da consegnare di un utente
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname dell'utente a cui salvare il messaggio
 * @return  puntatore alla lista dei messaggi da consegnare
 *          NULL se utente non trovato
 */
list_t *retrieve_user_msg(user_manager_t *user_manager, char *name);

/**
 * @function connected_user
 * @brief restituisce informazioni sulla connessione di un utente
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname dell'utente a cui salvare il messaggio
 * @return  >=0 fd della connessione sulla quale è collegato l'utente attualmente
 *          -1  se l'utente non è loggato
 *          -2  se l'utente non è registrato
 */
int connected_user(user_manager_t *user_manager, char *name);

/**
 * @function free_msg
 * @brief libera la memoria occupata da un messaggio
 *
 * @param msg puntatore all'area dove è allocato il messaggio
 */
void free_msg(void *msg);

/**
 * @function free_user
 * @brief libera la memoria occupata da un utente
 *
 * @param msg puntatore all'area dove è allocata la struttura utente
 */
void free_user(void *user);

/**
 * @function free_name_user
 * @brief libera la memoria occupata dal nickname di un utente
 *
 * @param msg puntatore all'area dove è allocato il nickname 
 */
void free_name_user(void *nick);

#endif /* USER_H_ */
