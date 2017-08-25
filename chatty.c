/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 *
 * @author Gaspare Ferraro 520549 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 * originale dell'autore  
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
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

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
 */
struct statistics chattyStats = {0, 0, 0, 0, 0, 0, 0};
// Mutex per l'accesso in Mutua Esclusione
static pthread_mutex_t mtx_stats = PTHREAD_MUTEX_INITIALIZER;

/* struttura che memorizza le configurazioni del server, 
 * struct serverConfiguration configuration.h
 */
struct serverConfiguration configuration = {0, 0, 0, 0, 0, NULL, NULL, NULL};

/**
 * @function usage
 * @brief stampa l'uso del file
 *
 * @param progname nome del programma in esecuzione
 */
static void usage(const char *progname) {
  fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
  fprintf(stderr, "  %s -f conffile\n", progname);
}

/**
 *  @struct worker_arg
 *  @brief argomento del thread
 *
 *  @var index indice del thread
 */
struct worker_arg {
  unsigned long index;
};

// Segnatura delle funzioni utilizzate

/**
 *  @struct update_stats
 *  @brief Aggiorna i nuovi valori delle statistiche
 *
 *  @var reg       nuovi utenti registrati
 *  @var on        nuovi utenti online
 *  @var msg_del   nuovi messaggi consegnati
 *  @var msg_pen   nuovi messaggio pendenti
 *  @var file_del  nuovi file consegnati
 *  @var file_pen  nuovi file pendenti
 *  @var err       nuovi errori
 */
void update_stats(int reg, int on, int msg_del, int msg_pen, int file_del,
                  int file_pen, int err);

/**
 *  @struct signals_handler
 *  @brief registra i segnali da ascoltare
 *
 */
int signals_handler();

/**
 *  @struct stop_server
 *  @brief richiede la terminazione del server
 *
 */
void stop_server();

/**
 *  @struct print_stats
 *  @brief Stampa le statistiche su file
 *
 */
void print_stats();

/**
 *  @struct worker
 *  @brief funzione iniziale di un thread
 *
 *  @var arg indice del thread
 */
void *worker(void *arg);

/**
 *  @struct execute
 *  @brief elabora un messaggio si un client
 *
 *  @var msg messaggio da elaborare
 *  @var client_fd fd del client da servire
 *  @return 0 operazione correttamente effettuata
 *         -1 errore 
 */
int execute(message_t msg, int client_fd);

// Flag di controllo per la terminazione dei Thread
int stopped;

// Set di FD per la select
fd_set set;

// Mutex per la scrittura dei FD da ascoltare
static pthread_mutex_t mtx_set = PTHREAD_MUTEX_INITIALIZER;

// Coda per la gestione delle richieste dei client
queue_t *Q;

// ThreadPool e argomenti dei Thread
pthread_t *threadPool;
struct worker_arg *threadArg;

// Gestore degli utenti
user_manager_t *user_manager;

// Gestore dei gruppi
group_manager_t *group_manager;

int main(int argc, char *argv[]) {

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
    printf("%s \t-> [%s]\n", "statFileName", configuration.statFileName);
  if (configuration.dirName != NULL)
    printf("%s \t-> [%s]\n", "dirName", configuration.dirName);
  if (configuration.unixPath != NULL)
    printf("%s \t-> [%s]\n", "unixPath", configuration.unixPath);
  printf("%s \t-> [%lu]\n", "maxHistMsgs", configuration.maxHistMsgs);
  printf("%s \t-> [%lu]\n", "maxFileSize", configuration.maxFileSize);
  printf("%s \t-> [%lu]\n", "maxMsgSize", configuration.maxMsgSize);
  printf("%s \t-> [%lu]\n", "threadsInPool", configuration.threadsInPool);
  printf("%s \t-> [%lu]\n", "maxConnections", configuration.maxConnections);

  printf("Registro segnali!\n");
  signals_handler();

  printf("Creo socket!\n");
  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sock_fd < 0) {
    perror("socket");
    return -1;
  }

  printf("Creo coda richieste...\n");
  Q = create_queue(configuration.maxConnections + 1);

  printf("Creo user manager...\n");
  user_manager = create_user_manager(configuration.maxHistMsgs,
                                     configuration.maxConnections);

  printf("Creo group manager...\n");
  group_manager = create_group_manager(configuration.maxConnections);


  printf("Bind socket!\n");
  struct sockaddr_un sa;
  strncpy(sa.sun_path, configuration.unixPath,
          strlen(configuration.unixPath) + 1);
  sa.sun_family = AF_UNIX;
  bind(sock_fd, (struct sockaddr *)&sa, sizeof(sa));

  printf("Listen socket!\n");
  listen(sock_fd, configuration.maxConnections);

  // Set dei fd da ascoltare
  int fd_c, fd_num = 0, fd ;
  fd_set rdset;

  if (sock_fd > fd_num) fd_num = sock_fd;
  FD_ZERO(&set);
  FD_SET(sock_fd, &set);

  printf("Creo ThreadPool!\n");
  threadPool =
      (pthread_t *)malloc(configuration.threadsInPool * sizeof(pthread_t));
  threadArg = (struct worker_arg *)malloc(configuration.threadsInPool *
                                          sizeof(struct worker_arg));

  for (int i = 0; i < configuration.threadsInPool; i++) {
    threadArg[i].index = i;
    printf("Creo Thread %d!\n", i);
    int ret =
        pthread_create(&threadPool[i], NULL, worker, (void *)&threadArg[i]);
    if (ret != 0) {
      printf("Errore creazione thread %d\n", i);
    }
  }

  // Select timeout
  struct timeval tv = {0, 1000};

  while (!stopped) {
    pthread_mutex_lock(&mtx_set);
    rdset = set;
    pthread_mutex_unlock(&mtx_set);


    int sel_ret = select(fd_num + 1, &rdset, NULL, NULL, &tv);
    if (sel_ret >= 0) {

      for (fd = 0; fd <= fd_num; fd++) {
        if (!FD_ISSET(fd, &rdset) ) continue;

        if (fd == sock_fd) {
          
          // Se supero il numero di connessioni non accetto
          int acc = 1;
          pthread_mutex_lock(&mtx_stats);
          if( chattyStats.nonline >= configuration.maxConnections ) acc = 0;
          pthread_mutex_unlock(&mtx_stats);
        
          if( !acc ) continue ;
 
          fd_c = accept(sock_fd, NULL, 0);
          printf("Accept FD = %d!\n", fd_c);

          // Aggiunge dalla lista di ascolto
          pthread_mutex_lock(&mtx_set);
          FD_SET(fd_c, &set);
          pthread_mutex_unlock(&mtx_set);

          if (fd_c > fd_num) fd_num = fd_c;
        } else {

          printf("Nuova richiesta %d!\n", fd);

          // Toglie dalla lista di ascolto
          pthread_mutex_lock(&mtx_set);
          FD_CLR(fd, &set);
          pthread_mutex_unlock(&mtx_set);

          // Pusha nella coda
          push_queue(Q, fd);

        }
      }
    }
  }

  // Aspetto i thread che terminino
  for (int i = 0; i < configuration.threadsInPool; i++) {
    printf("Join thread %d\n", i);
    pthread_join(threadPool[i], NULL);
  }

  // Pulizia finale
  printf("Chiudo fd...\n");
  for (fd = 0; fd <= fd_num; fd++) shutdown(fd, SHUT_RDWR); 

  printf("Chiudo sock_fd...\n");
  close(sock_fd);

  printf("Distruggo coda...\n");
  delete_queue(Q);

  printf("Distruggo usermanager...\n");
  destroy_user_manager(user_manager);

  printf("Distruggo groupmanager...\n");
  destroy_group_manager(group_manager);

  printf("Free threadPool...\n");
  free(threadPool);
  free(threadArg);

  printf("Free configuration string...\n");
  free(configuration.dirName);
  free(configuration.unixPath);
  free(configuration.statFileName);

  configuration.dirName = NULL;
  configuration.unixPath = NULL;
  configuration.statFileName = NULL;

  printf("Return\n");

  fflush(stdout);

  return 0;

}

void stop_server() {

  printf("KILL\n");
  stopped = 1;
  pthread_cond_broadcast(&(Q->cond));

}

void print_stats() {

  printf("PRINT STATS\n");
  FILE *fdstat;
  fdstat = fopen(configuration.statFileName, "a");
  if (fdstat != NULL) {
    pthread_mutex_lock(&mtx_stats);
    printStats(fdstat);
    pthread_mutex_unlock(&mtx_stats);
  }
  fclose(fdstat);

}

int signals_handler() {

  // Dichiaro le strutture
  struct sigaction exitHandler;
  struct sigaction statsHandler;
  struct sigaction pipeHandler;

  // Azzero il cotenuto delle strutture
  memset(&exitHandler, 0, sizeof(exitHandler));
  memset(&statsHandler, 0, sizeof(statsHandler));
  memset(&pipeHandler, 0, sizeof(pipeHandler));

  // Assegno le funzioni handler alle rispettive strutture
  stopped = 0;
  exitHandler.sa_handler = stop_server;
  statsHandler.sa_handler = print_stats;
  pipeHandler.sa_handler = SIG_IGN;

  // Registro i segnali per terminare il server
  if (sigaction(SIGINT, &exitHandler, NULL) == -1) {
    perror("sigaction");
    return -1;
  }
  if (sigaction(SIGQUIT, &exitHandler, NULL) == -1) {
    perror("sigaction");
    return -1;
  }
  if (sigaction(SIGTERM, &exitHandler, NULL) == -1) {
    perror("sigaction");
    return -1;
  }

  // Se nel fie di configurazione ho specificato StatFileName
  // Registro anche SIGUSR1
  if (configuration.statFileName != NULL &&
      strlen(configuration.statFileName) > 0) {
    if (sigaction(SIGUSR1, &statsHandler, NULL) == -1) {
      perror("sigaction");
      return -1;
    }
  }

  // ignoro SIGPIPE per evitare di essere terminato da una scrittura su un
  // socket chiuso
  if (sigaction(SIGPIPE, &pipeHandler, NULL) == -1) {
    perror("sigaction");
    return -1;
  }

  return 0;

}

void *worker(void *arg) {

  int index = ((struct worker_arg *)arg)->index;

  printf("\tTHREAD[%d] Start\n", index);
  message_t msg;
  memset(&msg, 0, sizeof(message_t));

  // Finchè non è richiesto il termine
  while (!stopped) {

    printf("\tTHREAD[%d] Aspetto richiesta dalla coda\n", index);
    int fd = pop_queue(Q);
    printf("\tTHREAD[%d] coda = %d\n", index, fd);

    // Se sono uscito perchè devo fermarmi
    if (stopped == 1) break;

    // Se qualcun'altro ha preso la richiesta al mio posto
    if (fd == -1) continue;

    // Leggo l'intestazione
    if (readHeader(fd, &(msg.hdr)) > 0) {
      printf("\tTHREAD[%d] Servo Client [%d]\n", index, fd);
      printf("\tTHREAD[%d] header(op=%d, sender=%s)\n", index, msg.hdr.op,
             msg.hdr.sender);
      printf("\tTHREAD[%d] Inizio execute\n", index);
      int ret = execute(msg, fd);
      printf("\tTHREAD[%d] Fine execute (esito=%d)\n", index, ret);
      free(msg.data.buf);
      msg.data.buf = NULL;
      if (ret == 0) {
        printf("\tTHREAD[%d] Esito positivo, rimetto in coda\n", index);
        pthread_mutex_lock(&mtx_set);
        FD_SET(fd, &set);
        pthread_mutex_unlock(&mtx_set);
      } else {
        printf("\tTHREAD[%d] Esito negativo, non rimetto in coda\n", index);
        if (disconnect_fd_user(user_manager, fd) == 0) {
          // Aggiorno stats
          update_stats(0, -1, 0, 0, 0, 0, 0);
        } 
      }
    } else {
      free(msg.data.buf);
      msg.data.buf = NULL;
      printf("\tTHREAD[%d] Errore lettura\n", index);
      if (disconnect_fd_user(user_manager, fd) == 0) {
        // Aggiorno stats
        update_stats(0, -1, 0, 0, 0, 0, 0);
      } 
    }
  }

  printf("\tTHREAD[%d] Stop\n", index);

  return NULL;

}

int execute(message_t received, int client_fd) {

  int tmp;
  message_t reply;

  op_t operation = received.hdr.op;
  char *sender = received.hdr.sender;

  // Se non c'è nessun sender termino
  if (sender == NULL || strlen(sender) == 0) {
    setHeader(&(reply.hdr), OP_FAIL, "");
    sendHeader(client_fd, &(reply.hdr));
    return -1;
  }

  printf("\t\t\tSENDER[%s] OP[%d]\n", sender, operation);

  free(received.data.buf);

  // Se c'è un errore nella lettura del buffer termino
  if (readData(client_fd, &(received.data)) < 0) {
    free(received.data.buf);
    received.data.buf = NULL;
    printf("\t\t\tErrore lettura dati");
    return -1;
  }

  // Eseguo le diverse operazioni
  switch (operation) {
/*****************************************************************************/
/*****************************************************************************/

    // richiesta di registrazione di un ninckname
    case REGISTER_OP: {

      // Se non c'è nessun gruppo con quel nome e riesco a registrarmi
      if ( exists_group(group_manager, sender) == -1 &&
           register_user(user_manager, sender) == 0) {
        // Provo a connettermi
        tmp = connect_user(user_manager, sender, client_fd);

        // Qualcuno è già connesso
        if (tmp == -1) {
          setHeader(&(reply.hdr), OP_NICK_ALREADY, "");
          tmp = sendHeader(client_fd, &(reply.hdr));
          if (tmp < 0) {
            free(received.data.buf);
            received.data.buf = NULL;
            return -1;
          }
        } 
        // Sono riusicto a connettermi      
        else {
          update_stats(1, 1, 0, 0, 0, 0, 0);
          // Ottengo la lista utenti e la mando all'utente
          char *buffer;
          int len = user_list(user_manager, &buffer);
          setHeader(&(reply.hdr), OP_OK, "");
          setData(&(reply.data), "", buffer, len * (MAX_NAME_LENGTH + 1));
          // Mando l'header
          tmp = sendHeader(client_fd, &(reply.hdr));
          if (tmp < 0) {
            free(received.data.buf);
            received.data.buf = NULL;
            free(buffer);
            return -1;
          }
          // Mando i dati
          tmp = sendData(client_fd, &(reply.data));
          if (tmp < 0) {
            free(received.data.buf);
            received.data.buf = NULL;
            free(buffer);
            return -1;
          }
          free(buffer);
        }
      } 
      // Non sono riuscito, il nick esisteva già
      else {
        update_stats(0, 0, 0, 0, 0, 0, 1);
        setHeader(&(reply.hdr), OP_NICK_ALREADY, "");
        tmp = sendHeader(client_fd, &(reply.hdr));
        if (tmp < 0) {
          free(received.data.buf);
          received.data.buf = NULL;
          return -1;
        }
      }
      free(received.data.buf);
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di connessione di un client
    case CONNECT_OP: {
      printf("\t\t\tINIZIO OP CONNECT SENDER[%s]\n", sender);
      // Provo a connettermi
      int tmp = connect_user(user_manager, sender, client_fd);
      printf("\t\t\tESITO CONNECT = %d\n", tmp);
      // Sono connesso
      if (tmp == 0) {
        char *buffer;
        int len = user_list(user_manager, &buffer);
        update_stats(0, 1, 0, 0, 0, 0, 0);

        // Invio la lista degli utenti attivi
        setHeader(&(reply.hdr), OP_OK, "");
        setData(&(reply.data), "", buffer, len * (MAX_NAME_LENGTH + 1));
        tmp = sendHeader(client_fd, &(reply.hdr));
        if (tmp < 0) {
          free(received.data.buf);
          received.data.buf = NULL;
          free(buffer);
          return -1;
        }
        tmp = sendData(client_fd, &(reply.data));
        if (tmp < 0) {
          free(received.data.buf);
          received.data.buf = NULL;
          free(buffer);
          return -1;
        }
        free(received.data.buf);
        received.data.buf = NULL;
        free(buffer);
      } 
      // Non sono riuscito a connetterm
      else {
        update_stats(0, 0, 0, 0, 0, 0, 1);
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");
        tmp = sendHeader(client_fd, &(reply.hdr));
        if (tmp < 0) {
          free(received.data.buf);
          received.data.buf = NULL;
          return -1;
        }
      }
      free(received.data.buf);
      printf("\t\t\tFINE OP CONNECT SENDER[%s]\n", sender);
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di invio di un messaggio testuale ad un nickname o groupname
    case POSTTXT_OP: {
      printf("\t\t\tINIZIO OP POSTTXT SENDER[%s]\n", sender);

      int len = received.data.hdr.len;
      printf("\t\t\tLEN = %d\n", len);
      char *receiver = received.data.hdr.receiver;
      printf("\t\t\tFD UTENTE[%s]\n", receiver);
      // Ottengo l'fd del ricevente
      int rec_fd = connected_user(user_manager, receiver);
      printf("\t\t\tFD UTENTE[%s] = %d\n", receiver, rec_fd);

      // Se il messaggio è troppo lungo
      if (len > configuration.maxMsgSize) {
        update_stats(0, 0, 0, 0, 0, 0, 1);
        setHeader(&(reply.hdr), OP_MSG_TOOLONG, "");
      } 
      // Ricevente registrato e connesso
      else if (rec_fd >= 0)  
      {
        update_stats(0, 0, 1, 0, 0, 0, 0);
        setHeader(&(reply.hdr), OP_OK, "");
        message_t *msg = copyMsg(&received);
        msg->hdr.op = TXT_MESSAGE;
        // Invio subito il messaggio
        sendRequest(rec_fd, msg);
        free(msg->data.buf);  
        free(msg);
      } 
      // Ricevente registrato ma non connessi
      else if (rec_fd == -1)  
      {
        update_stats(0, 0, 0, 1, 0, 0, 0);
        setHeader(&(reply.hdr), OP_OK, "");
        // Aggiungo il messaggio tra i suoi pendenti
        received.hdr.op = TXT_MESSAGE;
        post_msg(user_manager, receiver, &received);
      } 
      // Se il ricevente è un gruppo nel quale ci sono pure io
      else if( exists_group(group_manager, receiver) == 1 && 
              in_group(group_manager, receiver, sender) == 1 ) 
      {
        setHeader(&(reply.hdr), OP_OK, "");
        printf("\t\t\tPOSTTXT SENDER GRUPPO E REG[%s]\n", sender);
        char *list; 
        // ottengo la lista dei membri
        int size = members_group(group_manager, receiver, &list); 

        // La scorro
        for(int i=0; i<size; i++)
        {
          printf("- GROUP_MEMBER[%d][%s]\n",i,&list[i*( MAX_NAME_LENGTH + 1 )]);
          // Prendo il fd del membro attuale
          int tmp_fd = connected_user(user_manager, &list[i*( MAX_NAME_LENGTH + 1 )]);

          // Se è registrato e connesso
          if( tmp_fd >= 0 )
          {
            printf("- GROUP_MEMBER 1[%d][%s]\n",i,&list[i*( MAX_NAME_LENGTH + 1 )]);
            update_stats(0, 0, 1, 0, 0, 0, 0);
            message_t *msg = copyMsg(&received);
            msg->hdr.op = TXT_MESSAGE;
            // Invio subito il messaggio
            sendRequest(tmp_fd, msg);
            free(msg->data.buf);  
            free(msg);
          }
          // Se è registrato ma non connesso
          else if( tmp_fd == -1 )
          {
            update_stats(0, 0, 0, 1, 0, 0, 0);
            message_t *msg = copyMsg(&received);
            msg->hdr.op = TXT_MESSAGE;
            // Lo metto tra i suoi pendenti
            post_msg(user_manager, &list[i*( MAX_NAME_LENGTH + 1 )], msg);
          }
        } 

        free(list);
      }
      // Se il nick non esiste
      else 
      {
        update_stats(0, 0, 0, 0, 0, 0, 1);
        printf("\t\t\tPOSTTXT SENDER NO GRUPPO E REG[%s]\n", sender);
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");
      }

      int tmp = sendHeader(client_fd, &(reply.hdr));
      free(received.data.buf);
      printf("\t\t\tFINE OP POSTTXT SENDER[%s]\n", sender);
      if( tmp < 0 ) return -1;
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di invio di un messaggio testuale a tutti gli utenti
    case POSTTXTALL_OP: {
      printf("\t\t\tINIZIO OP POSTTXTALL SENDER[%s]\n", sender);
      int len = received.data.hdr.len;
      // Messaggio troppo lungo
      if (len > configuration.maxMsgSize) {
        update_stats(0, 0, 0, 0, 0, 0, 1);
        setHeader(&(reply.hdr), OP_MSG_TOOLONG, "");
        tmp = sendHeader(client_fd, &(reply.hdr));
        if (tmp < 0) {
          free(received.data.buf);
          received.data.buf = NULL;
          return -1;
        }
        free(received.data.buf);
        received.data.buf = NULL;
        return 0;
      }

      int *list;
      received.hdr.op = TXT_MESSAGE;
      // Invio a tutti i pendenti
      int pending = post_msg_all(user_manager, &received);
      printf("\t\t\tPENDING = %d\n", pending);
      // Ottengo la lista dei fd degli utenti attivi
      int sent = fd_list(user_manager, &list);
      printf("\t\t\tSENT = %d\n", sent);
      // Provo ad inviare a tutti gli utenti attivi
      for (int i = 0; i < sent; i++) {
        printf("\t\t\tINVIO = %d\n", i);
        message_t *msg = copyMsg(&received);
        msg->hdr.op = TXT_MESSAGE;
        printf("MANDO RICHIESTA (op=%d,from=%s,to=%s,len=%u,buf=%s)\n",
               msg->hdr.op, msg->hdr.sender, msg->data.hdr.receiver,
               msg->data.hdr.len, msg->data.buf);
        sendRequest(list[i], msg);
        free(msg->data.buf);
        free(msg);  
      }
      free(list);  
      update_stats(0, 0, sent, pending, 0, 0, 0);

      // Mando risposta al client
      setHeader(&(reply.hdr), OP_OK, "");
      tmp = sendHeader(client_fd, &(reply.hdr));
      if (tmp < 0) {
        free(received.data.buf);
        received.data.buf = NULL;
        return -1;
      }
      free(received.data.buf);
      printf("\t\t\tFINE OP POSTTXTALL SENDER[%s]\n", sender);
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di invio di un file ad un nickname o groupname
    case POSTFILE_OP: {
      printf("\t\t\tINIZIO OP POSTFILE SENDER[%s]\n", sender);

      printf("\t\t\tRECEIVED NAME[%s] SIZE[%d]\n", received.data.buf,
             received.data.hdr.len);
      message_data_t file_data;
      readData(client_fd, &file_data);
      printf("\t\t\tRECEIVED FILESIZE[%d]\n", file_data.hdr.len);
      // Se il file supera le dimensioni massime
      if (file_data.hdr.len > configuration.maxFileSize * 1024) {
        update_stats(0, 0, 0, 0, 0, 0, 1);
        // Invio errore troppo lungo
        setHeader(&(reply.hdr), OP_MSG_TOOLONG, "");
        tmp = sendHeader(client_fd, &(reply.hdr));
        if (tmp < 0) {
          free(file_data.buf);  
          free(received.data.buf);
          received.data.buf = NULL;
          return -1;
        }
      } 
      // File di dimensioni inviabiili
      else {
        // Pulisco il path
        int pathlen =
            (strlen(configuration.dirName) + strlen(received.data.buf) + 2);
        char *filename = malloc(pathlen * sizeof(char *));
        memset(filename, 0, pathlen);
        strcat(filename, configuration.dirName);
        strcat(filename, "/");
        char *lastSlash = strrchr(received.data.buf, '/');

        if (lastSlash != NULL) {
          lastSlash++;
          int nlen = received.data.hdr.len - (lastSlash - received.data.buf);
          strncat(filename, lastSlash, nlen);
        } else
          strncat(filename, received.data.buf, received.data.hdr.len);

        // Apro il file per scriverci il buffer
        FILE *fd;
        fd = fopen(filename, "wb");
        if (fd == NULL) {
          perror("open");
          fprintf(stderr, "ERRORE: aprendo il file %s\n", filename);
          fclose(fd);
          free(received.data.buf);
          received.data.buf = NULL;
          free(file_data.buf);  
          free(filename);       
          return -1;
        }

        // Provo a scrivere
        printf("\t\t\tAPERTO %s\n", filename);
        if (fwrite(file_data.buf, sizeof(char), file_data.hdr.len, fd) !=
            file_data.hdr.len)

        {
          perror("open");
          fprintf(stderr, "ERRORE: scrivendo il file %s\n", filename);
          fclose(fd);
          free(received.data.buf);
          received.data.buf = NULL;
          free(file_data.buf);  
          free(filename);       
          return -1;
        }

        fclose(fd);

        // Ottengo il fd del ricevemten
        char *receiver = received.data.hdr.receiver;
        int rec_fd = connected_user(user_manager, receiver);

        // Se è registrato e connesso
        if (rec_fd >= 0)  
        {
          // Gli mando una notifica di file pendente
          setHeader(&(reply.hdr), OP_OK, "");
          message_t *msg = copyMsg(&received);
          msg->hdr.op = FILE_MESSAGE;
          tmp = sendRequest(rec_fd, msg);
          free(msg->data.buf);  
          free(msg);
          update_stats(0, 0, 0, 0, 1, 0, 0);

        }
        // Se è registrato ma non connesso
        else if (rec_fd == -1)  
        {
          // Aggiungo ai messaggi pendent
          setHeader(&(reply.hdr), OP_OK, "");
          received.hdr.op = FILE_MESSAGE;
          post_msg(user_manager, receiver, &received);

          update_stats(0, 0, 0, 0, 0, 1, 0);

        }
        // Se il ricevente è un gruppo di cui io faccio parte
        else if( exists_group(group_manager, receiver) == 1 && 
                in_group(group_manager, receiver, sender) == 1 ) 
        {
          setHeader(&(reply.hdr), OP_OK, "");
          printf("\t\t\tPOSTFILE SENDER GRUPPO E REG[%s]\n", sender);
          // Ottengo la lista dei membri
          char *list; 
          int size = members_group(group_manager, receiver, &list); 
          for(int i=0; i<size; i++)
          {
            // Ottengo il fd del membro attuale
            int tmp_fd = connected_user(user_manager, &list[i*( MAX_NAME_LENGTH + 1 )]);
            // SE è connesso
            if( tmp_fd >= 0 )
            {
              update_stats(0, 0, 1, 0, 0, 0, 0);
              // Invio subito il messaggio di notifica
              message_t *msg = copyMsg(&received);
              msg->hdr.op = FILE_MESSAGE;
              sendRequest(tmp_fd, msg);
              free(msg->data.buf);  
              free(msg);
            }
            // Se è registrato ma non connesso
            else if( tmp_fd == -1 )
            {
              // Aggiungo ai file pendenti
              update_stats(0, 0, 0, 1, 0, 0, 0);
              message_t *msg = copyMsg(&received);
              msg->hdr.op = FILE_MESSAGE;
              post_msg(user_manager, &list[i*( MAX_NAME_LENGTH + 1 )], msg);
            }
          } 

          free(list);
        }
        // Il nick o groupname non esiste
        else
        {
          update_stats(0, 0, 0, 0, 0, 0, 1);
          setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");
        }

        // Invio l'esito
        tmp = sendHeader(client_fd, &(reply.hdr));
        free(file_data.buf);  
        free(filename);       
        if (tmp < 0) {
          free(received.data.buf);
          received.data.buf = NULL;
          return -1;
        }
      }
      free(received.data.buf);
      printf("\t\tFINE OP POSTFILE SENDER[%s]\n", sender);
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di recupero di un file
    case GETFILE_OP: {
      printf("\t\t\tINIZIO OP GETFILE SENDER[%s]\n", sender);

      // Ottengo il path locale
      int pathlen =
          (strlen(configuration.dirName) + strlen(received.data.buf) + 2);
      char *filename = malloc(pathlen * sizeof(char *));
      memset(filename, 0, pathlen);
      strcat(filename, configuration.dirName);
      strcat(filename, "/");
      strcat(filename, received.data.buf);

      // Lo apro
      char *mappedfile;
      int fd = open(filename, O_RDONLY);
  
      // Se c'è stato un errore
      if (fd < 0) {
        // Notifico che il file non esiste
        setHeader(&(reply.hdr), OP_NO_SUCH_FILE, "");
        tmp = sendHeader(client_fd, &(reply.hdr));
        if (tmp < 0) {
          free(filename);  
          free(received.data.buf);
          received.data.buf = NULL;
          return -1;
        }
      } 
      // Se il file è stato aperto correttamente
      else {

        // Controllo dimensione file
        struct stat st;
        if (stat(filename, &st) == -1 || !S_ISREG(st.st_mode)) {
          setHeader(&(reply.hdr), OP_NO_SUCH_FILE, "");
          tmp = sendHeader(client_fd, &(reply.hdr));
          if (tmp < 0) {
            free(filename);  
            free(received.data.buf);
            received.data.buf = NULL;
            return -1;
          }
        }

        int filesize = st.st_size;
  
        // Leggo il file
        mappedfile = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);

        // Errore mmap mando errore al client
        if (mappedfile == MAP_FAILED) {
          update_stats(0, 0, 0, 0, 0, 0, 1);
        
          setHeader(&(reply.hdr), OP_NO_SUCH_FILE, "");
          tmp = sendHeader(client_fd, &(reply.hdr));
          if (tmp < 0) {
            free(filename);  
            return -1;
          }
        } 
        // File mappato correttamentes
        else {
          close(fd);

          update_stats(0, 0, 0, 0, 1, -1, 0);

          // Invio header
          setHeader(&(reply.hdr), OP_OK, "");
          tmp = sendHeader(client_fd, &(reply.hdr));
          if (tmp < 0) {
            free(filename);  
            free(received.data.buf);
            received.data.buf = NULL;
            return -1;
          }

          // Invio dati
          setData(&(reply.data), "", mappedfile, filesize);
          tmp = sendData(client_fd, &(reply.data));
          if (tmp < 0) {
            free(filename);  
            free(received.data.buf);
            received.data.buf = NULL;
            return -1;
          }
          free(filename);  
        }
      }
      free(received.data.buf);
      printf("\t\t\tFINE OP GETFILE SENDER[%s]\n", sender);
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di recupero della history dei messaggi
    case GETPREVMSGS_OP: {
      printf("\t\t\tINIZIO OP GETPREVMSGS SENDER[%s]\n", sender);
      // Ottengo la lista con i messaggi pendenti
      list_t *prev = retrieve_user_msg(user_manager, sender);
      printf("\t\t\tGETPREVSMGS len=%ld\n", prev->cursize);
      // Se non è vuota
      if (prev != NULL) {
        // Invio il numero di messaggi pendenti
        setHeader(&(reply.hdr), OP_OK, "");
        setData(&(reply.data), "", (char *)&(prev->cursize), sizeof(size_t));
        sendRequest(client_fd, &reply);
        message_t *to_send;
        // Finchè ci sono messaggi pendenti
        while ((to_send = (message_t *)pop_list(prev)) != NULL) {
          printf("---------------------------------INVIO[%d][%s]\n",
                 to_send->hdr.op, to_send->data.buf);
          if (to_send->hdr.op == TXT_MESSAGE) {
            update_stats(0, 0, 1, -1, 0, 0, 0);
          } else {
            update_stats(0, 0, 0, 0, 1, -1, 0);
          }
          // Invio il messaggio e lo dealloco
          int tmp = sendRequest(client_fd, to_send);
          free(to_send->data.buf);  
          free(to_send);
          if (tmp < 0) {
            free(received.data.buf);
            received.data.buf = NULL;
            return -1;
          }
        }
      } 
      // Se la lista non esiste
      else {
        setHeader(&(reply.hdr), OP_FAIL, "");
        sendHeader(client_fd, &(reply.hdr));
      }
      free(received.data.buf);
      printf("\t\t\tFINE OP GETPREVMSGS SENDER[%s]\n", sender);
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di avere la lista di tutti gli utenti attualmente connessi
    case USRLIST_OP: {
      printf("\t\t\tINIZIO OP USRLIST SENDER[%s]\n", sender);
      // Dichiaro il buffer e chiamo la funzione
      char *buffer;
      int len = user_list(user_manager, &buffer);
      // Preparo il messaggio
      setHeader(&(reply.hdr), OP_OK, "");
      setData(&(reply.data), "", buffer, len * (MAX_NAME_LENGTH + 1));
      // Mando header
      tmp = sendHeader(client_fd, &(reply.hdr));
      if (tmp < 0) {
        free(received.data.buf);
        received.data.buf = NULL;
        return -1;
      } 
      // Mando dati
      tmp = sendData(client_fd, &(reply.data));
      if (tmp < 0) {
        free(received.data.buf);
        received.data.buf = NULL;
        return -1;
      }
      free(received.data.buf);
      printf("\t\t\tFINE OP USRLIST SENDER[%s]\n", sender);
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di deregistrazione di un nickname o groupname
    case UNREGISTER_OP: {
      printf("\t\t\tINIZIO OP UNREGISTER SENDER[%s]\n", sender);
      // Se riesco a disconnettermi
      if (disconnect_user(user_manager, sender) == 0) {
        update_stats(0, -1, 0, 0, 0, 0, 0);
        // Se riesco a deregistrare
        if (unregister_user(user_manager, sender) == 0) {
          update_stats(-1, 0, 0, 0, 0, 0, 0);
          setHeader(&(reply.hdr), OP_OK, "");
        }
        // Se non riesco a deregistrare 
        else {
          setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");
        }
      } 
      // Se non riesco a disconnettermi      
      else {
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");
      }
      // Invio risposta
      free(received.data.buf);
      sendHeader(client_fd, &(reply.hdr));
      printf("\t\t\tFINE OP UNREGISTER SENDER[%s]\n", sender);
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // OK
    // richiesta di disconnessione
    case DISCONNECT_OP: {
      printf("\t\t\tINIZIO OP DISCONNECT SENDER[%s]\n", sender);
      // Se riesco a disconnettermi
      if (disconnect_user(user_manager, sender) == 0) {
        update_stats(0, -1, 0, 0, 0, 0, 0);
        setHeader(&(reply.hdr), OP_OK, "");
      } 
      // Se non ci riesco 
      else {
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");
      }
      // Mando risposta
      sendHeader(client_fd, &(reply.hdr));
      free(received.data.buf);
      printf("\t\t\tFINE OP DISCONNECT SENDER[%s]\n", sender);
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di creazione di un gruppo
    case CREATEGROUP_OP: {

      printf("\t\t\tINIZIO OP CREATEGROUP SENDER[%s]\n", sender);
      char *receiver = received.data.hdr.receiver;
  
      // Cerco se c'è un utente con lo stesso nome
      int rec_fd = connected_user(user_manager, receiver);
      printf("\t\t\t\tCREO GROUP[%s]\n",receiver);

      // Se non c'è posso crearlo
      if( rec_fd == -2 )
      {
        int ret = create_group(group_manager, receiver);
        if( ret == 1 )
        {
          printf("\t\t\t\tCREO GROUP[%s]\n",receiver);
          char *member = (char*)malloc(sizeof(char)*(1+strlen(sender)));
          memset(member, 0, sizeof(char)*(1+strlen(sender)));
          strncpy(member, sender, sizeof(char)*(1+strlen(sender)));
          // Provo ad unirmi
          ret = join_group(group_manager, receiver, member);
          // Mi sono unito correttamente
          if( ret == 1 )
          {
            printf("\t\t\t\tJOIN GROUP[%s]\n",receiver);
            setHeader(&(reply.hdr), OP_OK, "");
          }
          // Non sono riuscito ad unirmi
          else
          {
            printf("\t\t\t\tNO GROUP[%s]\n",receiver);
            setHeader(&(reply.hdr), OP_FAIL, "");
          }  
        }
        // Non sono riuscito a creare il gruppo
        else
        {
          printf("\t\t\t\tNO CREO GROUP[%s]\n",receiver);
          setHeader(&(reply.hdr), OP_NICK_ALREADY, "");
        }
      }
      // Se c'è allora non posso crearlo
      else
      {
        printf("\t\t\t\tNO CREO GROUP[%s]\n",receiver);
        setHeader(&(reply.hdr), OP_NICK_ALREADY, "");
      }
      // Invio risposta
      int tmp = sendHeader(client_fd, &(reply.hdr));
      free(received.data.buf);
      printf("\t\t\tFINE OP CREATEGROUP SENDER[%s]\n", sender);
      if( tmp < 0 ) return -1;
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di aggiunta ad un gruppo
    case ADDGROUP_OP: {
      printf("\t\t\tINIZIO OP ADDGROUP SENDER[%s]\n", sender);
      char *receiver = received.data.hdr.receiver;
      char *member = (char*)malloc(sizeof(char)*(1+strlen(sender)));
      memset(member, 0, sizeof(char)*(1+strlen(sender)));
      strncpy(member, sender, sizeof(char)*(1+strlen(sender)));

      // Provo ad unirmi al gruppo
      int ret = join_group(group_manager, receiver, member);
      
      // Ci sono riuscito
      if( ret == 1 )
      {
        printf("\t\t\t\tSI JOIN[%s]\n",receiver);
        setHeader(&(reply.hdr), OP_OK, "");
      }
      // Il gruppo non esiste o ci sono già dentro
      else
      {
        printf("\t\t\t\tNO JOIN[%s]\n",receiver);
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");
      }

      // Invio risposta
      int tmp = sendHeader(client_fd, &(reply.hdr));
      free(received.data.buf);
      printf("\t\t\tFINE OP ADDGROUP SENDER[%s]\n", sender);
      if( tmp < 0 ) return -1;
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // richiesta di rimozione da un gruppo
    case DELGROUP_OP: {
      printf("\t\t\tINIZIO OP DELGROUP SENDER[%s]\n", sender);
      char *receiver = received.data.hdr.receiver;
      char *member = (char*)malloc(sizeof(char)*(1+strlen(sender)));
      memset(member, 0, sizeof(char)*(1+strlen(sender)));
      strncpy(member, sender, sizeof(char)*(1+strlen(sender)));

      // Provo a lasciare il gruppo
      int ret = leave_group(group_manager, receiver, member);
      free(member);

      // operazione effettuata correttamente
      if( ret == 1 )
      {
        printf("\t\t\t\tSI LEAVE[%s]\n",receiver);
        setHeader(&(reply.hdr), OP_OK, "");
      }
      // Non sono appartenente al gruppo o non esiste
      else
      {
        printf("\t\t\t\tNO LEAVE[%s]\n",receiver);
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");
      }
      // Invio risposta
      int tmp = sendHeader(client_fd, &(reply.hdr));
      free(received.data.buf);
      printf("\t\t\tFINE OP DELGROUP SENDER[%s]\n", sender);
      if( tmp < 0 ) return -1;
    } break;

/*****************************************************************************/
/*****************************************************************************/

    // Operazione conosciuta
    default: { 
      // Invio fail e termino
      setHeader(&(reply.hdr), OP_FAIL, "");
      int tmp = sendHeader(client_fd, &(reply.hdr));
      free(received.data.buf);
      if( tmp < 0 ) return -1;

    } break; 

  }

  printf("\t\t\tEND SENDER[%s] OP[%d]\n", sender, operation);
  return 0;

}

void update_stats(int reg, int on, int msg_del, int msg_pen, int file_del,
                  int file_pen, int err) {

  pthread_mutex_lock(&mtx_stats);
  chattyStats.nusers += reg;
  chattyStats.nonline += on;
  chattyStats.ndelivered += msg_del;
  chattyStats.nnotdelivered += msg_pen;
  chattyStats.nfiledelivered += file_del;
  chattyStats.nfilenotdelivered += file_pen;
  chattyStats.nerrors += err;
  pthread_mutex_unlock(&mtx_stats);

}
