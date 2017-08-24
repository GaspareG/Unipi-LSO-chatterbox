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
 * @file user.c
 * @brief Implementazione delle funzioni per la gestione
 *        degli utenti sul server
 */

#ifndef USER_C_
#define USER_C_

// Include necessari
#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <pthread.h>
#include <icl_hash.h>
#include <message.h>
#include <user.h>
#include <list.h>

/**
 * @function create_user_manager
 * @brief crea la struttura di gestione degli utenti
 *
 * @return puntatore all'user_manager creato
 * @param history_size dimensione della cronologia per ogni utente
 * @param nbuckets     numero dei bucket per le hashmap
 */
user_manager_t *create_user_manager(unsigned long history_size,
                                    unsigned long nbuckets) {
  if (history_size < 1) history_size = 1;
  if (nbuckets < 4) nbuckets = 4;
  // alloco in memoria i campi del manager
  user_manager_t *ret = (user_manager_t *)malloc(sizeof(user_manager_t));
  ret->mtx = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  ret->hash =
      icl_hash_create(nbuckets, string_hash_function, string_key_compare);
  ret->fd = icl_hash_create(nbuckets, ulong_hash_function, ulong_key_compare);
  ret->connected = 0;
  ret->history_size = history_size;
  // inizializzo il mutex
  pthread_mutex_init(ret->mtx, NULL);
  return ret;
}

/**
 * @function destroy_user_manager
 * @brief libera la memoria occupata dall'user_manager
 *
 * @param user_manager puntatore all'user_manager da liberare
 */
void destroy_user_manager(user_manager_t *user_manager) {
  // aspetto, distruggo e libero il mutex
  pthread_mutex_lock(user_manager->mtx);
  pthread_mutex_destroy(user_manager->mtx);
  free(user_manager->mtx);
  user_manager->mtx = NULL;
  // pulisco le hashmap
  icl_hash_destroy(user_manager->fd, NULL, NULL);
  icl_hash_destroy(user_manager->hash, NULL, free_user);
  free(user_manager);
  user_manager = NULL;
}

/**
 * @function register_user
 * @brief registra un nuovo utente
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname da registrare
 * @return  0 se utente registrato correttamente
 *          -1 per errore
 */
int register_user(user_manager_t *user_manager, char *name) {
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  // se l'utente non è già registrato
  if (icl_hash_find(user_manager->hash, (void *)name) == NULL) {
    ret = 0;
    // creo la struttura utente
    user_t *user = (user_t *)malloc(sizeof(user_t));
    strncpy(user->name, name, MAX_NAME_LENGTH + 1);
    user->history = create_list(user_manager->history_size);
    user->user_fd = -1;
    // e la inserisco nell'hashmap
    icl_entry_t *ins_ret =
        icl_hash_insert(user_manager->hash, user->name, (void *)user);
    if (ins_ret == NULL) ret = -1;
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

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
int connect_user(user_manager_t *user_manager, char *name, unsigned long fd) {
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  user_t *user = (user_t *)icl_hash_find(user_manager->hash, name);
  // se l'utente esiste e non è già loggato
  if (user != NULL && user->user_fd == -1) {
    ret = 0;
    // aggiorno il fd dell'utente
    user->user_fd = fd;
    // inserisco il suo fd nell'hashmao
    icl_entry_t *ins_ret = icl_hash_insert(user_manager->fd, &(user->user_fd),
                                           (void *)(user->name));
    if (ins_ret == NULL) {
      ret = -1;
    } else
      user_manager->connected = (user_manager->connected) + 1;
      // aumento il numero dei connessi
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

/**
 * @function unregister_user
 * @brief deregistra un utente registrato
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname da deregistrare
 * @return  0 se utente deregistrato correttamente
 *          -1 per errore
 */
int unregister_user(user_manager_t *user_manager, char *name) {
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  user_t *user = (user_t *)icl_hash_find(user_manager->hash, (void *)name);
  // se l'utente esiste
  if (user != NULL) {
    ret = 0;
    // distrutto solo la history utente
    icl_hash_delete(user_manager->hash, name, NULL, free_user);
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

/**
 * @function disconnect_user
 * @brief disconnette un utente registrato
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname da disconnettere
 * @return  0 se utente deregistrato correttamente
 *          -1 per errore
 */
int disconnect_user(user_manager_t *user_manager, char *name) {
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  user_t *user = (user_t *)icl_hash_find(user_manager->hash, (void *)name);
  // se l'utente esiste
  if (user != NULL) {
    ret = 0;
    // decremento il numero dei connessi
    user_manager->connected = (user_manager->connected) - 1;
    // elimino il suo fd della struttura utente e dall'hashmap
    user->user_fd = -1;
    icl_hash_delete(user_manager->fd, &(user->user_fd), NULL, NULL);
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

/**
 * @function disconnect_fd_user
 * @brief disconnette un utente registrato dato il suo fd
 *
 * @param user_manager puntatore all'user_manager
 * @param fd           fd dell'utente da disconnettere
 * @return  0 se utente deregistrato correttamente
 *          -1 per errore
 */
int disconnect_fd_user(user_manager_t *user_manager, unsigned long fd) {
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  char *username = (char *)icl_hash_find(user_manager->fd, (void *)&fd);
  // se il fd esiste
  if (username != NULL) {
    user_t *user =
        (user_t *)icl_hash_find(user_manager->hash, (void *)username);
    // se l'utente associato al fd esiste
    if (user != NULL) {
      ret = 0;
      // decremento il numero dei connessi
      user_manager->connected = (user_manager->connected) - 1;
      // elimino fd dalla struttura utente e dall'hashmap
      icl_hash_delete(user_manager->fd, &(user->user_fd), NULL, NULL);
      user->user_fd = -1;
    }
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

/**
 * @function user_list
 * @brief restituisce la lista degli utenti connessi attualmente
 *
 * @param user_manager puntatore all'user_manager
 * @param list         area dove allocare la stringa con la lista utenti
 * @return  n>=0 numero degli utenti attualmente collegati
 *          -1 per errore
 */
int user_list(user_manager_t *user_manager, char **list) {
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  ret = user_manager->connected;
  int i;
  icl_entry_t *j;
  char *kp;
  user_t *dp;
  // alloco e pulisco la stringa dove scrivere la lista degli utenti 
  *list = (char *)malloc(ret * (MAX_NAME_LENGTH + 1) * sizeof(char));
  memset(*list, 0, ret * (MAX_NAME_LENGTH + 1) * sizeof(char));
  char *tmp = *list;
  // scorro l'hashmap e inserisco solo gli utenti connessi
  icl_hash_foreach(user_manager->hash, i, j, kp, dp) {
    if (dp->user_fd != -1) {
      strncpy(tmp, dp->name, MAX_NAME_LENGTH + 1);
      tmp += MAX_NAME_LENGTH + 1;
    }
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

/**
 * @function fd_list
 * @brief restituisce la lista dei fd degli utenti connessi attualmente
 *
 * @param user_manager puntatore all'user_manager
 * @param list         area dove allocare la lista con i fd degli utenti
 * @return  n>=0 numero degli utenti attualmente collegati
 *          -1 per errore
 */
int fd_list(user_manager_t *user_manager, int **list) {
  int ret = 0;
  pthread_mutex_lock(user_manager->mtx);
  ret = user_manager->connected;
  int i;
  icl_entry_t *j;
  char *kp;
  user_t *dp;
  // alloco e pulisco l'array che conterrà la lista dei fd
  *list = (int *)malloc(ret * sizeof(int));
  memset(*list, 0, ret * sizeof(int));
  int tmp = 0;
  icl_hash_foreach(user_manager->hash, i, j, kp, dp) {
    if (dp->user_fd != -1) (*list)[tmp++] = dp->user_fd;
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

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
int post_msg(user_manager_t *user_manager, char *name, message_t *msg) {
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);

  user_t *user = (user_t *)icl_hash_find(user_manager->hash, (void *)name);
  // se l'utente esiste
  if (user != NULL) {
    ret = 0;
    // faccio push del messaggio nella history
    message_t *ret =
        (message_t *)push_list(user->history, (void *)copyMsg(msg));
    // se il push ha fatto scartare un vecchio messaggio, lo cancello
    if (ret != NULL) {
      free(ret->data.buf);
      free(ret);
    }
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

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
int post_msg_all(user_manager_t *user_manager, message_t *msg) {
  int ret = 0;
  pthread_mutex_lock(user_manager->mtx);
  int i;
  icl_entry_t *j;
  char *kp;
  user_t *dp;
  // per ogni utente non connesso, faccio push del messaggio da salvare
  icl_hash_foreach(user_manager->hash, i, j, kp, dp) {
    if (dp->user_fd != -1) continue;
    ret++;
    message_t *ret = (message_t *)push_list(dp->history, (void *)copyMsg(msg));
    if (ret != NULL) {
      free(ret->data.buf);
      free(ret);
    }
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

/**
 * @function retrieve_user_msg
 * @brief restituisce la lista dei messaggi da consegnare di un utente
 *
 * @param user_manager puntatore all'user_manager
 * @param name         nickname dell'utente a cui salvare il messaggio
 * @return  puntatore alla lista dei messaggi da consegnare
 *          NULL se utente non trovato
 */
list_t *retrieve_user_msg(user_manager_t *user_manager, char *name) {
  list_t *ret = NULL;
  pthread_mutex_lock(user_manager->mtx);
  user_t *user = (user_t *)icl_hash_find(user_manager->hash, (void *)name);
  // se l'utente esiste restituisco la lista dei messaggi pendenti
  if (user != NULL) {
    ret = user->history;
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

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
int connected_user(user_manager_t *user_manager, char *name) {
  int ret = -2;
  pthread_mutex_lock(user_manager->mtx);
  user_t *user = (user_t *)icl_hash_find(user_manager->hash, (void *)name);
  // se l'utente esiste restituisco il suo fd nella struttura utenti
  if (user != NULL) {
    ret = user->user_fd;
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

/**
 * @function free_msg
 * @brief libera la memoria occupata da un messaggio
 *
 * @param msg puntatore all'area dove è allocato il messaggio
 */
void free_msg(void *msg) {
  // libero prima il buffer dati e poi il messaggio
  message_t *tmp = (message_t *)msg;
  free(tmp->data.buf);
  tmp->data.buf = NULL;
  free(tmp);
  tmp = NULL;
}

/**
 * @function free_user
 * @brief libera la memoria occupata da un utente
 *
 * @param msg puntatore all'area dove è allocata la struttura utente
 */
void free_user(void *user) {
  // distruggo e libero l'hashmap della history e poi libero la struttura
  user_t *usr = (user_t *)user;
  destroy_list(usr->history, free_msg);
  free(usr->history);
  usr->history = NULL;
  free(usr);
  user = NULL;
}

/**
 * @function free_name_user
 * @brief libera la memoria occupata dal nickname di un utente
 *
 * @param msg puntatore all'area dove è allocato il nickname 
 */
void free_name_user(void *nick) { free((char *)nick); }

#endif /* USER_C_ */
