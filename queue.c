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
 * @file queue.c
 * @brief Implementazione delle funzioni per la gestione
 *        di una coda sincronizzata gestita mediante array circolare
 */

#ifndef QUEUE_C_
#define QUEUE_C_

#include <pthread.h>
#include <queue.h>
#include <stdlib.h>
#include <string.h>

/**
 * @function create_queue
 * @brief crea una coda dalla la dimensione
 *
 * @param size dimensione della coda
 * @return  puntatore alla coda creata
 */
queue_t* create_queue(int size) {
  // alloca e inizia i valori
  queue_t* q = (queue_t*)malloc(sizeof(queue_t));
  memset(q, 0, sizeof(queue_t));
  q->size = size;
  q->data = (int*)malloc(size * sizeof(int));
  q->front = 0;
  q->back = 0;
  // inizializza mutex e condition
  pthread_mutex_init(&(q->mtx), NULL);
  pthread_cond_init(&(q->cond), NULL);
  return q;
}

/**
 * @function push_queue
 * @brief aggiunge il valore alla coda
 *
 * @param q     coda nella quale inserire il valore
 * @param value valore da inserire
 * @return  >= 0 vaore inserito
 *          -1 coda piena
 */
int push_queue(queue_t* q, int value) {
  int ret = -1;
  pthread_mutex_lock(&(q->mtx));
  // Se la coda non è piena
  if (q->front != (q->back + 1) % (q->size)) {
    // inserisco il valore e scorro la coda
    q->data[q->back] = value;
    q->back = (q->back + 1) % q->size;
    ret = value;
    pthread_cond_signal(&(q->cond));
  }
  pthread_mutex_unlock(&(q->mtx));
  return ret;
}

/**
 * @function pop_queue
 * @brief togli un valore dalla coda
 *
 * @param q  coda dalla quale rimuovere
 * @return  >= 0 valore tolto
 *          -1 coda vuota
 */
int pop_queue(queue_t* q) {
  int ret = -1;
  pthread_mutex_lock(&(q->mtx));
  // se vuota mi metto in attesa
  if (q->front == q->back) pthread_cond_wait(&(q->cond), &(q->mtx));
  // Se non è vuota prendo il valore di fronte e scorro
  if (q->front != q->back) {
    ret = q->data[q->front];
    q->front = (q->front + 1) % q->size;
  }
  pthread_mutex_unlock(&(q->mtx));
  return ret;
}

/**
 * @function delete_queue
 * @brief libera la memoria occupata da una coda
 *
 * @param q coda da liberare
 */
void delete_queue(queue_t* q) {
  // aspetto mutex poi distruggo e libero la memoria
  pthread_mutex_lock(&(q->mtx));
  pthread_mutex_destroy(&(q->mtx));
  pthread_cond_destroy(&(q->cond));
  free(q->data);
  free(q);
}

#endif /* QUEUE_C_ */
