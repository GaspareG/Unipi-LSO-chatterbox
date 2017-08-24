 /*
 * chatterbox Progetto del corso di LSO 2017 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

#ifndef USER_C_
#define USER_C_

#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <pthread.h>
#include <icl_hash.h>
#include <message.h>
#include <user.h>
#include <list.h>

// Creo l'user manager
user_manager_t* create_user_manager(unsigned long history_size, unsigned long nbuckets)
{
  if( history_size < 1 ) history_size = 1;
  if( nbuckets < 4 ) nbuckets = 4;
  user_manager_t *ret = (user_manager_t*) malloc(sizeof(user_manager_t));
  ret->mtx = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  ret->hash = icl_hash_create(nbuckets, string_hash_function, string_key_compare);
  ret->fd = icl_hash_create(nbuckets, ulong_hash_function, ulong_key_compare);
  ret->connected = 0;
  ret->history_size = history_size;
  pthread_mutex_init(ret->mtx, NULL);
  return ret; 
}


// Distrugge l'user manager   
void destroy_user_manager(user_manager_t *user_manager)
{
  pthread_mutex_lock(user_manager->mtx);
  pthread_mutex_destroy(user_manager->mtx);
  free(user_manager->mtx);
  icl_hash_destroy(user_manager->hash, NULL, free_user);
  icl_hash_destroy(user_manager->fd, NULL, free_name_user); 
  free(user_manager);
}

// Registra un utente
int register_user(user_manager_t *user_manager, char *name)
{
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  if( icl_hash_find(user_manager->hash, (void*)name ) == NULL )
  {
    ret = 0 ;
    user_t *user = (user_t*) malloc(sizeof(user_t));
    strncpy(user->name, name, MAX_NAME_LENGTH+1);
    user->history = create_list(user_manager->history_size);
    //user->groups = create_list(0);
    user->user_fd = -1;
    icl_entry_t *ins_ret = icl_hash_insert(user_manager->hash, user->name, (void*)user);
    if( ins_ret == NULL ) ret = -1;    
  } 
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

// Connette un utente
int connect_user(user_manager_t *user_manager, char *name, unsigned long fd)
{
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  user_t *user = (user_t*) icl_hash_find(user_manager->hash, name );
  if( user != NULL && user->user_fd == -1)
  {
    ret = 0 ;
    user_manager->connected = (user_manager->connected)+1;
    user->user_fd = fd;
    icl_entry_t *ins_ret = icl_hash_insert(user_manager->fd, &(user->user_fd), (void*)(user->name));
    if( ins_ret == NULL ){
      //printf("connect_user[ERRORE FD] FD=%ld USER=%s HASH(%d)\n", user->user_fd, user->name, ulong_hash_function( &(user->user_fd))); 
      ret = -1; 
    } 
    //printf("connect_user[%s] trovato!\n", name);
  } 
  //else
  //  printf("connect_user[%s] non trovato!\n", name);
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

// Deregistra un utente
int unregister_user(user_manager_t *user_manager, char *name)
{
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  user_t *user = (user_t*) icl_hash_find(user_manager->hash, (void*)name );
  if( user != NULL )
  {
    ret = 0 ;    
    icl_hash_delete(user_manager->hash, name, NULL, NULL);
  } 
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

// Disconnette un utente
int disconnect_user(user_manager_t *user_manager, char *name)
{
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  user_t *user = (user_t*) icl_hash_find(user_manager->hash, (void*) name );
  if( user != NULL )
  {
    ret = 0 ;
    user_manager->connected = (user_manager->connected)-1;
    user->user_fd = -1;
    icl_hash_delete(user_manager->fd, &(user->user_fd), NULL, NULL);
  } 
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

// Disconnette un utente dato fd
int disconnect_fd_user(user_manager_t *user_manager, unsigned long fd)
{
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  char *username = (char*) icl_hash_find(user_manager->fd, (void*)&fd );
  if( username != NULL )
  {
    user_t *user = (user_t*) icl_hash_find(user_manager->hash, (void*) username );
    if( user != NULL )
    {
      ret = 0 ;
      user_manager->connected = (user_manager->connected)-1;
      icl_hash_delete(user_manager->fd, &(user->user_fd), NULL, NULL);
      user->user_fd = -1;
    } 
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

// Restituisce la lista degli utenti come stringa
int user_list(user_manager_t *user_manager, char **list)
{
  int ret = -1;
  // printf("user_list lock(%d)\n", ret);
  pthread_mutex_lock(user_manager->mtx);
  ret = user_manager->connected;
  // printf("user_list(%d)\n", ret);
  int i;
  icl_entry_t *j;
  char *kp;
  user_t *dp;
  *list = (char*) malloc( ret * (MAX_NAME_LENGTH+1) * sizeof(char) ); 
  memset(*list, 0, ret * (MAX_NAME_LENGTH+1) * sizeof(char));
  char *tmp = *list;
  icl_hash_foreach(user_manager->hash, i, j, kp, dp)
  {
    if( dp->user_fd != -1 )
    {
      strncpy(tmp, dp->name ,MAX_NAME_LENGTH+1);
      tmp += MAX_NAME_LENGTH+1;
    }   
  }
  // printf("user_list end(%d)\n", ret);
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

// Restituisce la lista di fd degli online
int fd_list(user_manager_t *user_manager, int **list)
{
  int ret = 0;
  // printf("user_list lock(%d)\n", ret);
  pthread_mutex_lock(user_manager->mtx);
  ret = user_manager->connected;
  // printf("user_list(%d)\n", ret);
  int i;
  icl_entry_t *j;
  char *kp;
  user_t *dp;
  *list = (int*) malloc( ret * sizeof(int) ); 
  memset(*list, 0, ret * sizeof(int));
  int tmp =0;
  icl_hash_foreach(user_manager->hash, i, j, kp, dp)
  {
    if( dp->user_fd != -1 )
      (*list)[tmp++] = dp->user_fd;
  }
  // printf("user_list end(%d)\n", ret);
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

// Post MSG
int post_msg(user_manager_t *user_manager, char *name, message_t *msg)
{
  int ret = -1;
  pthread_mutex_lock(user_manager->mtx);
  
  user_t *user = (user_t*) icl_hash_find(user_manager->hash, (void*) name );
  if( user != NULL )
  {
    printf("\t\t\t\tpost_msg(%s) trovato, msg = %s\n", name, msg->data.buf);
    ret = 0 ;
    push_list(user->history, (void*) copyMsg(msg));     
  } 
  else
    printf("\t\t\t\tpost_msg(%s) non trovato\n", name);
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

// Post MSG ALL
int post_msg_all(user_manager_t *user_manager, message_t *msg)
{
  int ret = 0;
  pthread_mutex_lock(user_manager->mtx);
  int i;
  icl_entry_t *j;
  char *kp;
  user_t *dp;
  icl_hash_foreach(user_manager->hash, i, j, kp, dp)
  {
    if( dp->user_fd != -1 ) continue;
    ret++;
    push_list(dp->history, (void*) copyMsg(msg));
  }
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}

// retrieve message list
int retrieve_user_msg(user_manager_t *user_manager, char *name, message_t **msg_list)
{ 
  int size = -1; 
  pthread_mutex_lock(user_manager->mtx);
  user_t *user = (user_t*) icl_hash_find(user_manager->hash, (void*)name );
	printf("\t\t\t\t\tOK %s\n",name);
  if( user != NULL )
  {
    size = (user->history)->cursize;
		printf("\t\t\t\t\tSIZE = %d\n",size);
    *msg_list = (message_t*) malloc( size * sizeof(message_t) );
    int tmp = 0;
    printf("\t\t\t\thistory(%s) msg[%d]\n", name, size);
    while( tmp < size )
    {
      msg_list[tmp] = (message_t*)pop_list(user->history);
      printf("\t\t\t\t- [%d] from[%s] [%s]\n", tmp, msg_list[tmp]->hdr.sender, msg_list[tmp]->data.buf);
      tmp++;    
    }
  } 
	printf("\t\t\t\t\tOK2 %s\n",name);
  pthread_mutex_unlock(user_manager->mtx);
  return size;
}

// is user connected
int connected_user(user_manager_t *user_manager, char *name)
{
  int ret = -2;
  pthread_mutex_lock(user_manager->mtx);
  user_t *user = (user_t*) icl_hash_find(user_manager->hash, (void*)name );
  if( user != NULL )
  {
    ret = user->user_fd;
  } 
  pthread_mutex_unlock(user_manager->mtx);
  return ret;
}


void free_name_user(void *name)
{
   free((char*) name);
}

void free_user(void *user)
{
  user_t *usr = (user_t*) user;
  destroy_list(usr->history);
	free(usr->history);
  free(usr);
}


#endif /* USER_C_ */
