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
 * @file group.c
 * @brief Implementazione delle funzioni per la gestione
 *        dei gruppi di utenti sul server
 */

#ifndef GROUP_C_
#define GROUP_C_

#include <pthread.h>
#include <config.h>
#include <group.h>
#include <user.h>

/**
 * @function create_group_manager
 * @brief 
 *
 * @param manager   puntatore al group_manager
 * @param history_size dimensione della cronologia per ogni utente
 * @param nbuckets     numero dei bucket per le hashmap
 * @return puntatore al group_manager creato
 */
group_manager_t *create_group_manager(unsigned long nbuckets) {
  // alloca il manager
  group_manager_t *ret = (group_manager_t *)malloc(sizeof(group_manager_t));
  ret->buckets = nbuckets;
  // crea hashmap e mutex
  ret->groups =
      icl_hash_create(nbuckets, string_hash_function, string_key_compare);
  ret->mtx = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  // inizializza mutex
  pthread_mutex_init(ret->mtx, NULL);
  return ret;
}

/**
 * @function destroy_group_manager
 * @brief libera della memoria il group manager
 *
 * @param manager   puntatore al group_manager
 */
void destroy_group_manager(group_manager_t *manager) {
  // aspetta il mutex, poi lo distrugge e libera la memoria
  pthread_mutex_lock(manager->mtx);
  pthread_mutex_destroy(manager->mtx);
  free(manager->mtx);
  manager->mtx = NULL;
  // distruggo l'hashmap e tutti i suoi valori
  icl_hash_destroy(manager->groups, NULL, free_group);
  free(manager);
}

/**
 * @function create_group
 * @brief crea un nuovo gruppo
 *
 * @param manager   puntatore al group_manager
 * @param groupname nome del gruppo da creare
 * @return 1 operazione effettuata correttamente
 *        -1 errore nello svolgimento dell'operazione
 */
int create_group(group_manager_t *manager, char *groupname) {
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *)icl_hash_find(manager->groups, (void *)groupname);
  // SE il gruppo non esiste
  if (group == NULL) {
    // Alloco e assegno i dati del gruppo
    group_t *to_add = (group_t *)malloc(sizeof(group_t));
    strncpy(to_add->groupname, groupname, MAX_NAME_LENGTH + 1);
    to_add->size = 0;
    to_add->member = icl_hash_create(manager->buckets, string_hash_function,
                                     string_key_compare);
    // e lo aggiungo alla hashmap
    if (icl_hash_insert(manager->groups, to_add->groupname, (void *)to_add) !=
        NULL) {
      ret = 1;
    }
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}

/**
 * @function join_group
 * @brief registra un utente in un gruppo
 *
 * @param manager   puntatore al group_manager
 * @param groupname nome del gruppo
 * @param username  nome dell'utente
 * @return 1 operazione effettuata correttamente
 *        -1 errore nello svolgimento dell'operazione
 */
int join_group(group_manager_t *manager, char *groupname, char *username) {
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *)icl_hash_find(manager->groups, (void *)groupname);
  // Se il gruppo esiste
  if (group != NULL) {
    char *name = (char *)icl_hash_find(group->member, (void *)username);
    // E l'utente non appartiene già al gruppo
    if (name == NULL) {
      // Lo inserisco nell'insieme e aumento la dimensione
      if (icl_hash_insert(group->member, (void *)username, (void *)username) !=
          NULL) {
        ret = 1;
        group->size = group->size + 1;
      }
    }
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}

/**
 * @function leave_group
 * @brief deregistra un utente da un gruppo
 *
 * @param manager   puntatore al group_manager
 * @param groupname nome del gruppo
 * @param username  nome dell'utente
 * @return 1 operazione effettuata correttamente
 *        -1 errore nello svolgimento dell'operazione
 */
int leave_group(group_manager_t *manager, char *groupname, char *username) {
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *)icl_hash_find(manager->groups, groupname);
  // Se il grppo esiste
  if (group != NULL) {
    char *name = (char *)icl_hash_find(group->member, (void *)username);
    // E l'utente ci appartiene
    if (name != NULL) {
      // Lo rimuovo dalla lista dei registrati al gruppo
      if (icl_hash_delete(group->member, username, free_name_user, NULL) == 0) {
        group->size = group->size - 1;
        ret = 1;
      }
    }
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}

/**
 * @function exists_group
 * @brief verifica l'esistenza di un gruppo 
 *
 * @param manager   puntatore al group_manager
 * @param groupname nome del gruppo
 * @return 1 se il gruppo esiste
 *        -1 se il gruppo non esiste
 */
int exists_group(group_manager_t *manager, char *groupname) {
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *)icl_hash_find(manager->groups, groupname);
  // Se il gruppo esiste
  if (group != NULL) {
    ret = 1;
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}

/**
 * @function in_group
 * @brief verifica la presenza di un utente in un gruppo
 *
 * @param manager   puntatore al group_manager
 * @param groupname nome del gruppo
 * @param username  nome dell'utente
 * @return 1 se l'utente appartiene al gruppo
 *        -1 se l'utente non appartiene al gruppo
 */
int in_group(group_manager_t *manager, char *groupname, char *username) {
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *)icl_hash_find(manager->groups, groupname);
   // Se il gruppo esiste
  if (group != NULL) {
    char *name = (char *)icl_hash_find(group->member, username);
    // Se l'utente ci appartiene
    if (name != NULL) ret = 1;
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}

/**
 * @function members_group
 * @brief restituisce la lista dei partecipanti di un gruppo
 *
 * @param manager   puntatore al group_manager
 * @param groupname nome del gruppo 
 * @param list      puntatore all'area di memoria dove 
 *                  verranno scritti i partecipanti al gruppo
 * @return >= 0 numero dei partecipanti al gruppo
 *           -1 se il gruppo non esiste
 */
int members_group(group_manager_t *manager, char *groupname, char **list) {
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *)icl_hash_find(manager->groups, groupname);
  // Se il gruppo esiste
  if (group != NULL) {
    ret = 0;
    int size = group->size;
    int i;
    icl_entry_t *j;
    char *kp, *dp;
    // Alloco e pulisco la stringa da restituire
    *list = (char *)malloc((MAX_NAME_LENGTH + 1) * size * sizeof(char));
    memset(*list, 0, (MAX_NAME_LENGTH + 1) * size * sizeof(char));
    char *tmp = *list;
    // Scorro la lista dei membri e la scrivo nella string
    icl_hash_foreach(group->member, i, j, kp, dp) {
      strncpy(tmp, dp, MAX_NAME_LENGTH + 1);
      ret++;
      tmp += MAX_NAME_LENGTH + 1;
    }
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}

/**
 * @function free_group
 * @brief libera la memoria occupata da un gruppo
 *
 * @param group puntatore al gruppo da liberare
 */
void free_group(void *group) {
  // Distruggo hashmap e valori, poi libero il gruppo
  group_t *g = (group_t *)group;
  icl_hash_destroy(g->member, NULL, free_name_user);
  free(g);
}

#endif /* GROUP_C_ */
