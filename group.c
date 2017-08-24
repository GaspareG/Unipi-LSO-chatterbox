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

/*
typedef struct  {
  char groupname[MAX_NAME_LENGTH+1];
  icl_hash_t *member; 
  
} group_t;

typedef struct {
  user_manager_t *user_manager;
  pthread_mutex_t *mtx;
  icl_hash_t *groups;  
} group_manager_t ;
*/

// Creazione del group manager
group_manager_t* create_group_manager(user_manager_t* usr_mng, unsigned long nbuckets)
{
  group_manager_t *ret = (group_manager_t*) malloc( sizeof(group_manager_t) );
  ret->user_manager = usr_mng;
  ret->buckets = nbuckets;
  ret->groups = icl_hash_create(nbuckets, string_hash_function, string_key_compare);
  ret->mtx = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(ret->mtx, NULL);
  return ret;
}
  
// Distruzione del group manager
void destroy_group_manager(group_manager_t* manager)
{
  pthread_mutex_lock(manager->mtx);
  pthread_mutex_destroy(manager->mtx);
  free(manager->mtx);
  manager->mtx = NULL;
  manager->user_manager = NULL;
  icl_hash_destroy(manager->groups, NULL, free_group);
  free(manager);
}
  
// Creazione di un gruppo 
int create_group(group_manager_t* manager, char *groupname)
{
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *) icl_hash_find(manager->groups, (void*)groupname);
  if( group == NULL )
  {
    group_t *to_add = (group_t*) malloc( sizeof(group_t) );
    strncpy(to_add->groupname, groupname, MAX_NAME_LENGTH+1);
    to_add->size = 0 ;  
    to_add->member = icl_hash_create(manager->buckets, string_hash_function, string_key_compare);
    if( icl_hash_insert(manager->groups, to_add->groupname, (void*)to_add) != NULL )
    {
      ret = 1;
    }
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}

// Adesione di un utente ad un gruppo
int join_group(group_manager_t* manager, char *groupname, char *username)
{
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *) icl_hash_find(manager->groups, (void*)groupname);
  if( group != NULL )
  {
    char *name = (char*) icl_hash_find(group->member, (void*)username);
    if( name == NULL ) 
    {
      if( icl_hash_insert(group->member, (void*) username, (void*)username) != NULL )
      {
       ret = 1;
        group->size = group->size + 1 ;
      }
    }  
    icl_hash_dump(stdout,group->member);
    fflush(stdout);  
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}

// Abbandono di un utente di un gruppo
int leave_group(group_manager_t* manager, char *groupname, char *username)
{
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *) icl_hash_find(manager->groups, groupname);
  if( group != NULL )
  {
    char *name = (char*) icl_hash_find(group->member, (void*)username);
    if( name != NULL ) 
    {
      if( icl_hash_delete(group->member, username, free_name_user, NULL) == 0 ) 
      {
        group->size = group->size - 1 ;
        ret = 1;
      }
     
    }
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}


// Controllo se utente il gruppo
int exists_group(group_manager_t* manager, char *groupname)
{
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *) icl_hash_find(manager->groups, groupname);
  if( group != NULL )
  {
    ret = 1;
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}


// Controllo se utente in gruppo
int in_group(group_manager_t* manager, char *groupname, char *username)
{
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *) icl_hash_find(manager->groups, groupname);
  if( group != NULL )
  {
    char *name = (char*) icl_hash_find(group->member, username);
    if( name != NULL ) ret = 1;
  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}

// Invio di un messaggio in gruppo
int members_group(group_manager_t* manager, char *groupname, char **list)
{
  int ret = -1;
  pthread_mutex_lock(manager->mtx);
  group_t *group = (group_t *) icl_hash_find(manager->groups, groupname);
  if( group != NULL )
  {
    ret = 0 ;
    int size = group->size;
    int i;
    icl_entry_t *j;
    char *kp, *dp;
    *list = (char*) malloc( ( MAX_NAME_LENGTH + 1 ) * size * sizeof(char));
    memset(*list, 0, ( MAX_NAME_LENGTH + 1 ) * size * sizeof(char));
    char *tmp = *list;
    icl_hash_foreach(group->member, i, j, kp, dp) 
    {
      strncpy(tmp, dp, MAX_NAME_LENGTH + 1);
      ret++;
      tmp += MAX_NAME_LENGTH + 1;
    }

  }
  pthread_mutex_unlock(manager->mtx);
  return ret;
}

void free_group(void *group)
{
  group_t *g = (group_t*) group;
  icl_hash_destroy(g->member, NULL, free_name_user);
  free(g);  
}

#endif /* GROUP_C_ */
