/*
 * chatterbox Progetto del corso di LSO 2017 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <pthread.h>

typedef struct {
  int* data;
  int size;
  int front;
  int back;
  pthread_mutex_t mtx;
  pthread_cond_t cond;
} queue_t;

queue_t* create_queue(int size);

int push_queue(queue_t* q, int value);

int pop_queue(queue_t* q);

void delete_queue(queue_t *q);
    
#endif /* QUEUE_H_ */
