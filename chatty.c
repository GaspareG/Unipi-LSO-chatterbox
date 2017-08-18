/*
 * membox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

/* inserire gli altri include che servono */
#include <errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "ops.h"
#include "message.h"
#include "config.h"
#include "configuration.h"
#include "stats.h"
#include "connections.h"

/* struttura che memorizza le statistiche del server, struct statistics
 * e' definita in stats.h.
 *
 */
struct statistics chattyStats = {0, 0, 0, 0, 0, 0, 0};
struct serverConfiguration configuration = {0, 0, 0, 0, 0, NULL, NULL, NULL};
fd_set set;

static void usage(const char* progname) {
  fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
  fprintf(stderr, "  %s -f conffile\n", progname);
}

struct worker_t {
  int empty;
};

struct worker_arg {
  unsigned long index;
  struct worker_t data;
};

struct queue {
  int* data;
  int size;
  int front;
  int back;
};

// Segnatura delle funzioni utilizzate
void signalsHandler();
void stopServer();
void printStatistics();
void* worker(void* arg);

int execute(message_t msg, int client_fd);

struct queue* createQueue(int size);
int push(struct queue* q, int value);
int pop(struct queue* q);
void deleteQueue(struct queue* q);

int stopped;

struct queue* Q;

pthread_t* threadPool;
struct worker_arg* threadArg;

int main(int argc, char* argv[]) {
  // Controllo uso corretto degli qrgomenti
  if (argc < 3 || strncmp(argv[1], "-f", 2) != 0) {
    usage(argv[0]);
    return 0;
  }

  // Parso il file di configurazione
  printf("Leggo configurazioni...\n");
  readConfig(argv[2], &configuration);

  // Stampo le configurazioni lette
  printf("Configurazioni lette:\n");
  if (configuration.statFileName != NULL)
    printf("%s -> [%s]\n", "statFileName", configuration.statFileName);
  if (configuration.dirName != NULL)
    printf("%s -> [%s]\n", "dirName", configuration.dirName);
  if (configuration.unixPath != NULL)
    printf("%s -> [%s]\n", "unixPath", configuration.unixPath);
  printf("%s -> [%lu]\n", "maxHistMsgs", configuration.maxHistMsgs);
  printf("%s -> [%lu]\n", "maxFileSize", configuration.maxFileSize);
  printf("%s -> [%lu]\n", "maxMsgSize", configuration.maxMsgSize);
  printf("%s -> [%lu]\n", "threadsInPool", configuration.threadsInPool);
  printf("%s -> [%lu]\n", "maxConnections", configuration.maxConnections);
  fflush(stdout);

  // Registro handler per i segnali
  printf("Registro segnali!\n");
  signalsHandler();
  printf("Segnali registrati!\n");

  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sock_fd < 0) {
    perror("socket");
    return -1;
  }

  // Creo i Thread
  threadPool =
      (pthread_t*)malloc(configuration.threadsInPool * sizeof(pthread_t));
  threadArg = (struct worker_arg*)malloc(configuration.threadsInPool *
                                         sizeof(struct worker_arg));

  for (int i = 0; i < configuration.threadsInPool; i++) {
    threadArg[i].index = i;

    int ret =
        pthread_create(&threadPool[i], NULL, worker, (void*)&threadArg[i]);
    if (ret != 0) {
    }
  }

  struct sockaddr_un sa;
  strncpy(sa.sun_path, configuration.unixPath,
          strlen(configuration.unixPath) + 1);
  sa.sun_family = AF_UNIX;
  bind(sock_fd, (struct sockaddr*)&sa, sizeof(sa));

  listen(sock_fd, configuration.maxConnections);

  int fd_c, fd_num = 0, fd;
  fd_set rdset;

  if (sock_fd > fd_num) fd_num = sock_fd;
  FD_ZERO(&set);
  FD_SET(sock_fd, &set);

  Q = createQueue(configuration.maxConnections + 1);

  while (!stopped) {
    rdset = set;

    if (select(fd_num + 1, &rdset, NULL, NULL, NULL) == -1) {
      // ERRORE
    } else {
      for (fd = 0; fd <= fd_num; fd++) {
        if (!FD_ISSET(fd, &rdset)) continue;

        if (fd == sock_fd) {
          // New client
          fd_c = accept(sock_fd, NULL, 0);
          FD_SET(fd_c, &set);
          if (fd_c > fd_num) fd_num = fd_c;
        } else {
          // New request
          FD_CLR(fd, &set);
          // Pusha nella coda
          push(Q, fd);
        }
      }
    }
  }

  for (int i = 0; i < configuration.threadsInPool; i++) {
    pthread_join(threadPool[i], NULL);
  }
  free(threadPool);
  free(threadArg);
  return 0;
}

void stopServer() {
  printf("KILL\n");
  stopped = 1;
  fflush(stdout);
}

/**
 * @function printStatistics
 * @brief Handler del segnale SIGUSR1, appende le statistiche
 *        nel file specificato nelle configurazioni
 */
void printStatistics() {
  FILE* fdstat;
  fdstat = fopen(configuration.statFileName, "a");
  if (fdstat != NULL) {
    chattyStats.nusers += 1;
    printStats(fdstat);
  }
  fclose(fdstat);
}

/**
 * @function signalsHandler
 * @brief Registra gli handler per i segnali richiesti
 */
void signalsHandler() {
  // Dichiaro le strutture
  struct sigaction exitHandler;
  struct sigaction statsHandler;

  // Azzero il cotenuto delle strutture
  memset(&exitHandler, 0, sizeof(exitHandler));
  memset(&statsHandler, 0, sizeof(statsHandler));

  // Assegno le funzioni handler alle rispettive strutture
  stopped = 0;
  exitHandler.sa_handler = stopServer;
  statsHandler.sa_handler = printStatistics;

  // Registro i segnali per terminare il server
  sigaction(SIGINT, &exitHandler, NULL);
  sigaction(SIGQUIT, &exitHandler, NULL);
  sigaction(SIGTERM, &exitHandler, NULL);

  // Se nel fie di configurazione ho specificato StatFileName
  // Registro anche SIGUSR1
  if (configuration.statFileName != NULL &&
      strlen(configuration.statFileName) > 0)
    sigaction(SIGUSR1, &statsHandler, NULL);

  // Ignoro i segnali di SIGPIPE
  signal(SIGPIPE, SIG_IGN);
}

void* worker(void* arg) {
  int index = ((struct worker_arg*)arg)->index;
//  struct worker_t data = ((struct worker_arg*)arg)->data;

//  data.empty = 0;
  printf("START %d\n", index);
  message_t* msg = (message_t*)malloc(sizeof(message_t));

  while (!stopped) {
    int fd = pop(Q);

    int ret = readMsg(fd, msg);
    if (ret != -1) execute(*msg, fd);

    FD_SET(fd, &set);
  }

  return NULL;
}

int execute(message_t msg, int client_fd) {
  op_t operation = msg.hdr.op;

  switch (operation) {
    // richiesta di registrazione di un ninckname
    case REGISTER_OP:
      // TODO
      break;

    // richiesta di connessione di un client
    case CONNECT_OP:
      // TODO
      break;

    // richiesta di invio di un messaggio testuale ad un nickname o groupname
    case POSTTXT_OP:
      // TODO
      break;

    // richiesta di invio di un messaggio testuale a tutti gli utenti
    case POSTTXTALL_OP:
      // TODO
      break;

    // richiesta di invio di un file ad un nickname o groupname
    case POSTFILE_OP:
      // TODO
      break;

    // richiesta di recupero di un file
    case GETFILE_OP:
      // TODO
      break;

    // richiesta di recupero della history dei messaggi
    case GETPREVMSGS_OP:
      // TODO
      break;

    // richiesta di avere la lista di tutti gli utenti attualmente connessi
    case USRLIST_OP:
      // TODO
      break;

    // richiesta di deregistrazione di un nickname o groupname
    case UNREGISTER_OP:
      // TODO
      break;

    // richiesta di disconnessione
    case DISCONNECT_OP:
      // TODO
      break;

    // richiesta di creazione di un gruppo
    case CREATEGROUP_OP:
      // TODO
      break;

    // richiesta di aggiunta ad un gruppo
    case ADDGROUP_OP:
      // TODO
      break;

    // richiesta di rimozione da un gruppo
    case DELGROUP_OP:
      // TODO
      break;

    default:
      break;
  }

  return 1;
}

struct queue* createQueue(int size) {
  struct queue* q = (struct queue*)malloc(sizeof(struct queue));
  memset(q, 0, sizeof(struct queue));
  q->size = size;
  q->data = (int*)malloc(size * sizeof(int));
  return q;
}

int push(struct queue* q, int value) {
  if (q->front == q->back) return -1;
  q->data[q->back] = value;
  q->back = (q->back + 1) % q->size;
  return value;
}

int pop(struct queue* q) {
  if (q->front == q->back) return -1;
  int ret = q->data[q->front];
  q->front = (q->front + 1) % q->size;
  return ret;
}

void deleteQueue(struct queue* q) {
  free(q->data);
  free(q);
}
