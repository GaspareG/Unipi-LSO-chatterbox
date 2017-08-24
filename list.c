 /*
 * chatterbox Progetto del corso di LSO 2017 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

#ifndef LIST_C_
#define LIST_C_

#include <string.h>
#include <list.h>
#include <stdlib.h>

list_t* create_list(unsigned long size)
{
  list_t *ret = (list_t*) malloc(sizeof(list_t));
  ret->size = size;
  ret->cursize = 0;
	ret->front = NULL;
  ret->back = NULL;
  return ret;
}

void push_list(list_t *lst, void *data)
{
  if( lst->size > 0 && lst->cursize == lst->size ) pop_list(lst);
  list_node_t *to_add = (list_node_t*) malloc(sizeof(list_node_t));
	
  to_add->data = data;
  to_add->next = NULL;

  if( lst->front == NULL && lst->back == NULL ) lst->front = to_add;
  if( lst->back != NULL ) (lst->back)->next = to_add;
  lst->back = to_add;
  lst->cursize++;
}

void* pop_list(list_t *lst)
{
	if( lst->cursize == 0 ) return NULL;
	void *ret = (void*) (lst->front)->data;
	if( lst->cursize == 1 ) 
	{
	  	lst->front = lst->back = NULL;
	}
	else
	{
		lst->front = (lst->front)->next;
	}
	lst->cursize--;
	return ret;
}

void destroy_list(list_t *lst)
{
	char *tmp ;
	tmp = NULL;
	while( (tmp = pop_list(lst)) != NULL ) free(tmp);
	free(lst);
}

#endif /* LIST_C_ */
