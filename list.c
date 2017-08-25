/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file list.c
 * @brief Implementazione delle funzioni per le operazioni
 *        di una coda mediante lista linkata
 *
 * @author Gaspare Ferraro 520549 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 * originale dell'autore  
 */

#ifndef LIST_C_
#define LIST_C_

#include <string.h>
#include <list.h>
#include <stdlib.h>
#include <message.h>

/**
 *  @struct create_list
 *  @brief crea una lista linkata
 *
 *  @var size dimensione massima della lista ( 0 = infinita )
 *  @return puntatore alla lista create
 */
list_t *create_list(unsigned long size) {
  list_t *ret = (list_t *)malloc(sizeof(list_t));
  ret->size = size;
  ret->cursize = 0;
  ret->front = NULL;
  ret->back = NULL;
  return ret;
}

/**
 *  @struct push_list
 *  @brief libera la lista della memoria
 *
 *  @var lst lista da dove fare il push
 *  @var data informazione da inserire
 *  @return puntatore al nodo rimosso se lista piena
 *          NULL se la coda non è piena o non ha limite di dimensione
 */
list_node_t *push_list(list_t *lst, void *data) {
  list_node_t *ret = NULL;
  if (lst->size > 0 && lst->cursize == lst->size) ret = pop_list(lst);

  list_node_t *to_add = (list_node_t *)malloc(sizeof(list_node_t));
  memset(to_add, 0, sizeof(list_node_t));

  to_add->data = data;
  to_add->next = NULL;

  if (lst->front == NULL && lst->back == NULL)
    lst->front = to_add;
  else
    (lst->back)->next = to_add;

  lst->back = to_add;
  lst->cursize++;
  return ret;
}

/**
 *  @struct pop_list
 *  @brief estra il primo elemento della list
 *
 *  @var lst       lista da dove fare il pop
 *  @return puntatore all'elemento tolto
 *          NULL se lista vuota
 */
void *pop_list(list_t *lst) {
  void *ret = NULL;
  if (lst->cursize != 0)
  {
    ret = (lst->front)->data;
    list_node_t *old = lst->front;
    if (lst->cursize == 1)
      lst->front = lst->back = NULL;
    else
      lst->front = (lst->front)->next;
    lst->cursize--;
    free(old);
  }
  return ret;
}

/**
 *  @struct destroy_list
 *  @brief libera la lista della memoria
 *
 *  @var lst       puntatore alla lista da liberare
 *  @var free_data puntatore alla funzione per liberare i dati dei nodi
 */
void destroy_list(list_t *lst, void (*free_data)(void *)) {
  void *tmp;
  tmp = NULL;
  while ((tmp = pop_list(lst)) != NULL) free_data(tmp);
}

#endif /* LIST_C_ */
