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
#include "icl_hash.h"
#include "user.h"
#include "group.h"
#include "queue.h"

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

/* funzioni hash per per l'hashing di stringhe */
static inline unsigned int string_hash_function( void *key ) {

    unsigned long hash = 5381;
    int c;
    unsigned char *str = (unsigned char*) key;
    while ( (c = *str++) )
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static inline int string_key_compare( void *key1, void *key2  ) {
    return strcmp( (char*) key1, (char*) key2 );
}


struct worker_arg {
  unsigned long index;
};

// Segnatura delle funzioni utilizzate
int signals_handler();
void stop_server();
void print_stats();
void* worker(void* arg);

int execute(message_hdr_t msg, int client_fd);

int stopped;

static pthread_mutex_t mtx_set = PTHREAD_MUTEX_INITIALIZER;

queue_t* Q;

pthread_t* threadPool;
struct worker_arg* threadArg;

static pthread_mutex_t mtx_user = PTHREAD_MUTEX_INITIALIZER;
icl_hash_t *hash_user;

// TODO
// static pthread_mutex_t mtx_group = PTHREAD_MUTEX_INITIALIZER;
// icl_hash_t *hash_group;

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
  signals_handler();
  printf("Segnali registrati!\n");

  Q = create_queue(configuration.maxConnections + 1);
  
  hash_user = icl_hash_create(configuration.maxConnections, string_hash_function, string_key_compare);
  // hash_group = icl_hash_create(configuration.maxConnections, string_hash_function, string_key_compare);

  printf("Creo socket!\n");
  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sock_fd < 0) {
    perror("socket");
    return -1;
  }

  threadPool = (pthread_t*)malloc(configuration.threadsInPool * sizeof(pthread_t));
  threadArg = (struct worker_arg*)malloc(configuration.threadsInPool *
                                         sizeof(struct worker_arg));

  

  struct sockaddr_un sa;
  strncpy(sa.sun_path, configuration.unixPath,
          strlen(configuration.unixPath) + 1);
  sa.sun_family = AF_UNIX;

  printf("Bind socket!\n");
  bind(sock_fd, (struct sockaddr*)&sa, sizeof(sa));

  printf("Listen socket!\n");
  listen(sock_fd, configuration.maxConnections);

  int fd_c, fd_num = 0, fd;
  fd_set rdset;

  if (sock_fd > fd_num) fd_num = sock_fd;
  FD_ZERO(&set);
  FD_SET(sock_fd, &set);

  for (int i = 0; i < configuration.threadsInPool; i++) {
    threadArg[i].index = i;

    printf("Creo Thread %d!\n", i);
    int ret = pthread_create(&threadPool[i], NULL, worker, (void*)&threadArg[i]);
    if( ret == -1 )
    {
      printf("ERRORE CREAZIONE THREAD %d\n", i);
    } 
  }

  while (!stopped) {
    pthread_mutex_lock(&mtx_set); 
    rdset = set;
    pthread_mutex_unlock(&mtx_set); 
    printf("Iterazione!\n");

    // TODO add timeout
    if (select(fd_num + 1, &rdset, NULL, NULL, NULL) == -1) {
      // ERRORE
    } else {
      for (fd = 0; fd <= fd_num; fd++) {
        if (!FD_ISSET(fd, &rdset)) continue;

        if (fd == sock_fd) {
          // New client
          fd_c = accept(sock_fd, NULL, 0);
          printf("Accept FD = %d!\n", fd_c);
          
          pthread_mutex_lock(&mtx_set); 
          FD_SET(fd_c, &set);
          pthread_mutex_unlock(&mtx_set); 
          
          if (fd_c > fd_num) fd_num = fd_c;
        } else {
          // New request
          printf("Richiesta %d!\n", fd);
          
          pthread_mutex_lock(&mtx_set); 
          FD_CLR(fd, &set);
          pthread_mutex_unlock(&mtx_set); 

          // Pusha nella coda
          push_queue(Q, fd);
          printf("Richiesta incodata %d!\n", fd);
        }
      }
    }
  }

  for (int i = 0; i < configuration.threadsInPool; i++) {
    printf("Join thread %d\n", i);
    pthread_join(threadPool[i], NULL);
  }
  free(threadPool);
  free(threadArg);
  return 0;
}

void stop_server() {
  printf("KILL\n");
  stopped = 1;
  fflush(stdout);
}

/**
 * @function print_stats
 * @brief Handler del segnale SIGUSR1, appende le statistiche
 *        nel file specificato nelle configurazioni
 */
void print_stats() {
  FILE* fdstat;
  fdstat = fopen(configuration.statFileName, "a");
  if (fdstat != NULL) {
    chattyStats.nusers += 1;
    printStats(fdstat);
  }
  fclose(fdstat);
}

/**
 * @function signals_handler
 * @brief Registra gli handler per i segnali richiesti
 */
int signals_handler() {
  // Dichiaro le strutture
  struct sigaction exitHandler;
  struct sigaction statsHandler;
  struct sigaction pipeHandler;

  // Azzero il cotenuto delle strutture
  memset(&exitHandler, 0, sizeof(exitHandler));
  memset(&statsHandler, 0, sizeof(statsHandler));
  memset(&pipeHandler,0,sizeof(pipeHandler));    

  // Assegno le funzioni handler alle rispettive strutture
  stopped = 0;
  exitHandler.sa_handler = stop_server;
  statsHandler.sa_handler = print_stats;
  pipeHandler.sa_handler = SIG_IGN;

  // Registro i segnali per terminare il server
  if( sigaction(SIGINT, &exitHandler, NULL)  == -1 ) {   
    perror("sigaction");
    return -1;
  } 
  if( sigaction(SIGQUIT, &exitHandler, NULL)  == -1 ) {   
    perror("sigaction");
    return -1;
  } 
  if( sigaction(SIGTERM, &exitHandler, NULL)  == -1 ) {   
    perror("sigaction");
    return -1;
  } 

  // Se nel fie di configurazione ho specificato StatFileName
  // Registro anche SIGUSR1
  if (configuration.statFileName != NULL &&
      strlen(configuration.statFileName) > 0)
  {
    if( sigaction(SIGUSR1, &statsHandler, NULL)  == -1 ) {   
      perror("sigaction");
      return -1;
    } 
  }

  // ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket chiuso
  if ( sigaction(SIGPIPE,&pipeHandler,NULL)  == -1 ) {   
    perror("sigaction");
    return -1;
  } 

  return 0;
}

void* worker(void* arg) {
  int index = ((struct worker_arg*)arg)->index;

  printf("\tTHREAD[%d] START\n", index);
  message_hdr_t hdr;

  while (!stopped) {

    // TODO condition variable
    int fd = pop_queue(Q);

    if( fd == -1 ) continue;

    printf("\tTHREAD[%d] Servo Client %d\n", index, fd);

    int ret = readHeader(fd, &hdr);
    printf("\tTHREAD[%d] RET = %d\n", index, ret);
    printf("\tTHREAD[%d] OP = %d\n", index, hdr.op);
    printf("\tTHREAD[%d] SENDER = %s\n", index, hdr.sender);

    execute(hdr, fd);

    pthread_mutex_lock(&mtx_set);  
    FD_SET(fd, &set);
    pthread_mutex_unlock(&mtx_set);  
  }

  printf("\tTHREAD[%d] STOP\n", index);
  return NULL;
}

int execute(message_hdr_t hdr, int client_fd) {
  message_t reply;

  op_t operation = hdr.op;
  char *sender = hdr.sender;

  switch (operation) {

    // richiesta di registrazione di un ninckname
    case REGISTER_OP: {      
      if( register_user(hash_user, &mtx_user, sender) == 0 )
      {
        setHeader(&(reply.hdr), OP_OK, "");      
        char *buffer;
        int len = user_list(hash_user, &mtx_user, &buffer);
        setData(&(reply.data), "", buffer, len);
        sendHeader(client_fd, &(reply.hdr));
        sendData(client_fd, &(reply.data));
      }
      else
      {
        setHeader(&(reply.hdr), OP_NICK_ALREADY, "");        
        sendHeader(client_fd, &(reply.hdr));
      }
    } break;

    // richiesta di connessione di un client
    case CONNECT_OP: {
      if( connect_user(hash_user, &mtx_user, sender, client_fd) == 0 )
      {
        setHeader(&(reply.hdr), OP_OK, "");      
        char *buffer;
        int len = user_list(hash_user, &mtx_user, &buffer);
        setData(&(reply.data), "", buffer, len);
        sendHeader(client_fd, &(reply.hdr));
        sendData(client_fd, &(reply.data));
      }
      else
      {
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");        
        sendHeader(client_fd, &(reply.hdr));
      }
    } break;

    // richiesta di invio di un messaggio testuale ad un nickname o groupname
    case POSTTXT_OP: {
      readData(client_fd, &(reply.data));  

      int len = reply.data.hdr.len;
      char *receiver = reply.data.hdr.receiver;
      char *buffer = reply.data.buf;
      int rec_fd = connected_user(hash_user, &mtx_user, receiver);

      if( len > configuration.maxMsgSize )
      {
        setHeader(&(reply.hdr), OP_MSG_TOOLONG, "");        
      }
      else if( rec_fd >= 0 ) // registrato e connesso
      {
        setHeader(&(reply.hdr), OP_OK, "");  
        message_t msg;
        setHeader(&(msg.hdr), TXT_MESSAGE, sender); 
        setData(&(msg.data), receiver, buffer, len);
        sendRequest(rec_fd, &msg);
      }
      else if( rec_fd == -1 ) // registrato ma non connesso
      {
        setHeader(&(reply.hdr), OP_OK, "");  
        user_msg_t msg;
        msg.type = 0;
        msg.buffer = buffer;
        msg.len = len;
        post_msg(hash_user, &mtx_user, receiver, msg);
      } 
      else // non registrato
      {
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");        
      }

      sendHeader(client_fd, &(reply.hdr));        
        
    } break;

    // richiesta di invio di un messaggio testuale a tutti gli utenti
    case POSTTXTALL_OP: {
      // TODO
    } break;

    // richiesta di invio di un file ad un nickname o groupname
    case POSTFILE_OP: {
      // TODO
    } break;

    // richiesta di recupero di un file
    case GETFILE_OP: {
      // TODO
    } break;

    // richiesta di recupero della history dei messaggi
    case GETPREVMSGS_OP: {
      message_t *list;
      int len = retrieve_user_msg(hash_user, &mtx_user, sender, &list);
      if( len >= 0 )
      {
        setHeader(&(reply.hdr), OP_OK, "");
        setData(&(reply.data), "", (char*)&len, sizeof(size_t));
        sendRequest(client_fd, &reply);
        for(int i=0; i<len; i++)
          sendRequest(client_fd, &list[i]);
      } 
      else
      {
        setHeader(&(reply.hdr), OP_FAIL, "");
        sendHeader( client_fd, &(reply.hdr) );
      } 
    } break;

    // richiesta di avere la lista di tutti gli utenti attualmente connessi
    case USRLIST_OP: {
      setHeader(&(reply.hdr), OP_OK, "");      
      char *buffer;
      int len = user_list(hash_user, &mtx_user, &buffer);
      setData( &(reply.data), "", buffer, len);
      sendHeader( client_fd, &(reply.hdr));
      sendData(client_fd, &(reply.data));
    } break;

    // richiesta di deregistrazione di un nickname o groupname
    case UNREGISTER_OP: {     
      if( unregister_user(hash_user, &mtx_user, sender) == 0 )
        setHeader(&(reply.hdr), OP_OK, "");      
      else
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");        
      
      sendHeader(client_fd, &(reply.hdr));
    } break;

    // richiesta di disconnessione
    case DISCONNECT_OP: {
      if( disconnect_user(hash_user, &mtx_user, sender) == 0 )
        setHeader(&(reply.hdr), OP_OK, "");      
      else
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");        
      
      sendHeader(client_fd, &(reply.hdr));
    } break;

    // richiesta di creazione di un gruppo
    case CREATEGROUP_OP: {
      // TODO
    } break;

    // richiesta di aggiunta ad un gruppo
    case ADDGROUP_OP: {
      // TODO
    } break;

    // richiesta di rimozione da un gruppo
    case DELGROUP_OP: {
      // TODO
    } break;

    default: {
    
    } break;
  }

  return 1;
}


