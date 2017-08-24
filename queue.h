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
 * @file queue.h
 * @brief Definizione delle strutture e funzioni per la gestione
 *        di una coda sincronizzata
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <pthread.h>

/**
 *  @struct queue_t
 *  @brief coda implemenetata mediante array circolare
 *
 *  @var data  array contenente i dati
 *  @var size  dimensione dell'array
 *  @var front indice testa
 *  @var back  indice coda
 *  @var mtx   mutex per la m.e.
 *  @var cond  condition per aspettare un nuovo elemento in coda
 */
typedef struct {
  int* data;
  int size;
  int front;
  int back;
  pthread_mutex_t mtx;
  pthread_cond_t cond;
} queue_t;

/**
 * @function create_queue
 * @brief crea una coda dalla la dimensione
 *
 * @param size dimensione della coda
 * @return  puntatore alla coda creata
 */
queue_t* create_queue(int size);

/**
 * @function push_queue
 * @brief aggiunge il valore alla coda
 *
 * @param q     coda nella quale inserire il valore
 * @param value valore da inserire
 * @return  >= 0 vaore inserito
 *          -1 coda piena
 */
int push_queue(queue_t* q, int value);

/**
 * @function pop_queue
 * @brief togli un valore dalla coda
 *
 * @param q  coda dalla quale rimuovere
 * @return  >= 0 valore tolto
 *          -1 coda vuota
 */
int pop_queue(queue_t* q);

/**
 * @function delete_queue
 * @brief libera la memoria occupata da una coda
 *
 * @param q coda da liberare
 */
void delete_queue(queue_t* q);

#endif /* QUEUE_H_ */
