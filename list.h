 /*
 * chatterbox Progetto del corso di LSO 2017 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

#ifndef LIST_H_
#define LIST_H_

#include <string.h>
#include <config.h>
#include <pthread.h>
#include <icl_hash.h>
#include <message.h>

typedef struct list_node_t {
  void *data;
  struct list_node_t *next;	
} list_node_t;

typedef struct {
  list_node_t *front;
  list_node_t *back;
  unsigned long cursize;
  unsigned long size; // 0 = no limit
} list_t;

list_t* create_list(unsigned long size);

list_node_t* push_list(list_t *lst, void *data);

void* pop_list(list_t *lst);

void destroy_list(list_t *lst, void (*free_data)(void*));

#endif /* LIST_H_ */
