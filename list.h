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
 * @file list.h
 * @brief Dichiarazioni strutture e funzioni per le operazioni
 *        su una coda
 */

#ifndef LIST_H_
#define LIST_H_

#include <string.h>
#include <config.h>
#include <pthread.h>
#include <icl_hash.h>
#include <message.h>

/**
 *  @struct list_node_t
 *  @brief nodo della lista
 *
 *  @var data informazione del nodo
 *  @var next puntatore all'elemento successivo
 */
typedef struct list_node_t {
  void *data;
  struct list_node_t *next;
} list_node_t;

/**
 *  @struct list_t
 *  @brief lista linkata
 *
 *  @var puntatore alla testa
 *  @var puntatore alla coda
 *  @var dimensione corrente
 *  @var dimensione massima
 */
typedef struct {
  list_node_t *front;
  list_node_t *back;
  unsigned long cursize;
  unsigned long size;  // 0 = no limit
} list_t;

/**
 *  @struct create_list
 *  @brief crea una lista linkata
 *
 *  @var size dimensione massima della lista ( 0 = infinita )
 *  @return puntatore alla lista create
 */
list_t *create_list(unsigned long size);

/**
 *  @struct push_list
 *  @brief libera la lista della memoria
 *
 *  @var lst lista da dove fare il push
 *  @var data informazione da inserire
 *  @return puntatore al nodo rimosso se lista piena
 *          NULL se la coda non è piena o non ha limite di dimensione
 */
list_node_t *push_list(list_t *lst, void *data);

/**
 *  @struct pop_list
 *  @brief estra il primo elemento della list
 *
 *  @var lst       lista da dove fare il pop
 *  @return puntatore all'elemento tolto
 *          NULL se lista vuota
 */
void *pop_list(list_t *lst);

/**
 *  @struct destroy_list
 *  @brief libera la lista della memoria
 *
 *  @var lst       puntatore alla lista da liberare
 *  @var free_data puntatore alla funzione per liberare i dati dei nodi
 */
void destroy_list(list_t *lst, void (*free_data)(void *));

#endif /* LIST_H_ */
