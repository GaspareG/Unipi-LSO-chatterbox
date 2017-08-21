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

// Registra un utente
int register_user(icl_hash_t *hash, pthread_mutex_t *mtx, char *name)
{
    int ret = -1;
    pthread_mutex_lock(mtx);
    if( icl_hash_find(hash, name) != NULL )
    {
        user_t *to_add = (user_t*) malloc( sizeof(user_t) );
        
        icl_hash_insert(hash, name, (void*) to_add);  
    }
    pthread_mutex_unlock(mtx);
    return ret;
}

// Connette un utente
int connect_user(icl_hash_t *hash, pthread_mutex_t *mtx, char *name, unsigned long fd)
{
    int ret = -1;
    pthread_mutex_lock(mtx);
    // TODO
    pthread_mutex_unlock(mtx);
    return ret;
}

// Deregistra un utente
int unregister_user(icl_hash_t *hash, pthread_mutex_t *mtx, char *name)
{
    int ret = -1;
    pthread_mutex_lock(mtx);
    // TODO
    pthread_mutex_unlock(mtx);
    return ret;
}

// Disconnette un utente
int disconnect_user(icl_hash_t *hash, pthread_mutex_t *mtx, char *name)
{
    int ret = -1;
    pthread_mutex_lock(mtx);
    // TODO
    pthread_mutex_unlock(mtx);
    return ret;
}

// Restituisce la lista degli utenti come stringa
int user_list(icl_hash_t *hash, pthread_mutex_t *mtx, char **list)
{
    int ret = -1;
    pthread_mutex_lock(mtx);
    // TODO
    pthread_mutex_unlock(mtx);
    return ret;
}

// Post MSG
int post_msg(icl_hash_t *hash, pthread_mutex_t *mtx, char *name, user_msg_t msg)
{
    int ret = -1;
    pthread_mutex_lock(mtx);
    // TODO
    pthread_mutex_unlock(mtx);
    return ret;
}

// Post MSG ALL
int post_msg_all(icl_hash_t *hash, pthread_mutex_t *mtx, user_msg_t msg)
{
    int ret = -1;
    pthread_mutex_lock(mtx);
    // TODO
    pthread_mutex_unlock(mtx);
    return ret;
}

// retrieve message list
int retrieve_user_msg(icl_hash_t *hash, pthread_mutex_t *mtx, char *name, message_t **msg_list)
{
    int ret = -1;
    pthread_mutex_lock(mtx);
    // TODO
    pthread_mutex_unlock(mtx);
    return ret;
}

// is user connected
int connected_user(icl_hash_t *hash, pthread_mutex_t *mtx, char *name)
{
    int ret = -1;
    pthread_mutex_lock(mtx);
    // TODO
    pthread_mutex_unlock(mtx);
    return ret;
}

#endif /* USER_C_ */
