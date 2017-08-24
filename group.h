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

/**
 *  @struct group_t
 *  @brief Informazioni di un gruppo
 *
 *  @var groupname nome del gruppo
 *  @var member    set dei membri del gruppo
 *  @var size      numero dei membri del gruppo
 */
typedef struct {
  char groupname[MAX_NAME_LENGTH + 1];
  icl_hash_t *member;
  int size;
} group_t;

/**
 *  @struct group_t
 *  @brief Informazioni di un gruppo
 *
 *  @var groupname nome del gruppo
 *  @var member    set dei membri del gruppo
 *  @var size      numero dei membri del gruppo
 */
typedef struct {
  pthread_mutex_t *mtx;
  icl_hash_t *groups;
  unsigned long buckets;
} group_manager_t;

/**
 * @function create_group_manager
 * @brief 
 *
 * @param manager   puntatore al group_manager
 * @param history_size dimensione della cronologia per ogni utente
 * @param nbuckets     numero dei bucket per le hashmap
 * @return puntatore al group_manager creato
 */
group_manager_t *create_group_manager(unsigned long nbuckets);

/**
 * @function destroy_group_manager
 * @brief libera della memoria il group manager
 *
 * @param manager   puntatore al group_manager
 */
void destroy_group_manager(group_manager_t *manager);

/**
 * @function create_group
 * @brief crea un nuovo gruppo
 *
 * @param manager   puntatore al group_manager
 * @param groupname nome del gruppo da creare
 * @return 1 operazione effettuata correttamente
 *        -1 errore nello svolgimento dell'operazione
 */
int create_group(group_manager_t *manager, char *groupname);

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
int join_group(group_manager_t *manager, char *groupname, char *username);

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
int leave_group(group_manager_t *manager, char *groupname, char *username);

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
int in_group(group_manager_t *manager, char *groupname, char *username);

/**
 * @function exists_group
 * @brief verifica l'esistenza di un gruppo 
 *
 * @param manager   puntatore al group_manager
 * @param groupname nome del gruppo
 * @return 1 se il gruppo esiste
 *        -1 se il gruppo non esiste
 */
int exists_group(group_manager_t *manager, char *groupname);

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
int members_group(group_manager_t *manager, char *groupname, char **list);

/**
 * @function free_group
 * @brief libera la memoria occupata da un gruppo
 *
 * @param group puntatore al gruppo da liberare
 */
void free_group(void *group);

#endif /* GROUP_H_ */
