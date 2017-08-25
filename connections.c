/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file connections.c
 * @brief Implementazione delle funzioni per la comunicazione
 *        tra client e server
 *
 * @author Gaspare Ferraro 520549 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 * originale dell'autore  
 */

#ifndef CONNECTIONS_C_
#define CONNECTIONS_C_

#define MAX_RETRIES 10
#define MAX_SLEEPING 3
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX 64
#endif

#include <message.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <connections.h>
#include <stdio.h>
#include <config.h>

/**
 * @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server
 *
 * @param path Path del socket AF_UNIX
 * @param ntimes numero massimo di tentativi di retry
 * @param secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char *path, unsigned int ntimes, unsigned int secs) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (fd == -1) return -1;

  struct sockaddr_un sa;
  strncpy(sa.sun_path, path, UNIX_PATH_MAX);
  sa.sun_family = AF_UNIX;

  if (ntimes > MAX_RETRIES) ntimes = MAX_RETRIES;
  if (secs > MAX_SLEEPING) secs = MAX_SLEEPING;
  // prova a connettersi ntimes volte, aspettando secs secondi dopo ogni tentativo
  while (ntimes-- && connect(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
    if (errno == ENOENT)
      sleep(secs);
    else
      return -1;
  }

  return fd;
}

/**
 * @function readBuffer
 * @brief Legge un buffer 
 *
 * @param fd     descrittore della connessione
 * @param buffer area di memoria dove leggere i dati letti
 * @param length lunghezza del buffer da leggere
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readBuffer(long connfd, char *buffer, unsigned int length) {
  char *tmp = buffer;
  while (length > 0) {
    int red = read(connfd, tmp, length);
    if (red < 0) return -1;
    tmp += red;
    length -= red;
  }
  return 1;
}

/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readHeader(long connfd, message_hdr_t *hdr) {
  memset(hdr, 0, sizeof(message_hdr_t));
  int tmp = read(connfd, hdr, sizeof(message_hdr_t));
  if (tmp < 0) return -1;
  return 1;
}

/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readData(long connfd, message_data_t *data) {
  memset(data, 0, sizeof(message_data_t));
  int tmp = read(connfd, &(data->hdr), sizeof(message_data_hdr_t));
  if (tmp < 0) return -1;
  if (data->hdr.len == 0) {
    data->buf = NULL;
  } else {
    data->buf = (char *)malloc(data->hdr.len * sizeof(char));
    memset(data->buf, 0, data->hdr.len * sizeof(char));
    tmp = readBuffer(connfd, data->buf, data->hdr.len);
    if (tmp < 0) {
      free(data->buf);
      return -1;
    }
  }
  return 1;
}

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readMsg(long connfd, message_t *msg) {
  memset(msg, 0, sizeof(message_t));
  int tmp = readHeader(connfd, &(msg->hdr));
  if (tmp < 0) return -1;
  tmp = readData(connfd, &(msg->data));
  if (tmp < 0) return -1;
  return 1;
}

/**
 * @function sendBuffer
 * @brief Invia un buffer 
 *
 * @param fd     descrittore della connessione
 * @param buffer area di memoria dove scrivere i dati letti
 * @param length lunghezza del buffer da scriveres
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int sendBuffer(long connfd, char *buffer, unsigned int length) {
  while (length > 0) {
    int wrote = write(connfd, buffer, length);
    if (wrote < 0) return -1;
    buffer += wrote;
    length -= wrote;
  }
  return 1;
}

/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendRequest(long connfd, message_t *msg) {
  int tmp = sendHeader(connfd, &(msg->hdr));
  if (tmp < 0) return -1;
  tmp = sendData(connfd, &(msg->data));
  if (tmp < 0) return -1;
  return 1;
}

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendData(long connfd, message_data_t *data) {
  int tmp = write(connfd, &(data->hdr), sizeof(message_data_hdr_t));
  if (tmp < 0) return -1;
  tmp = sendBuffer(connfd, data->buf, data->hdr.len);
  if (tmp < 0) return -1;
  return 1;
}

/**
 * @function sendHeader
 * @brief Invia l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int sendHeader(long connfd, message_hdr_t *msg) {
  int tmp = write(connfd, msg, sizeof(message_hdr_t));
  if (tmp < 0) return -1;
  return 1;
}

#endif /* CONNECTIONS_C_ */
