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
 *
 */
static pthread_mutex_t mtx_stats = PTHREAD_MUTEX_INITIALIZER;
struct statistics chattyStats = {0, 0, 0, 0, 0, 0, 0};

struct serverConfiguration configuration = {0, 0, 0, 0, 0, NULL, NULL, NULL};

static void usage(const char* progname) {
  fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
  fprintf(stderr, "  %s -f conffile\n", progname);
}


struct worker_arg {
  unsigned long index;
};

// Segnatura delle funzioni utilizzate
void update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err);
int signals_handler();
void stop_server();
void print_stats();
void* worker(void* arg);
int execute(message_hdr_t msg, int client_fd);

int stopped;

static pthread_mutex_t mtx_set = PTHREAD_MUTEX_INITIALIZER;
fd_set set;

queue_t* Q;

pthread_t* threadPool;
struct worker_arg* threadArg;

user_manager_t *user_manager;

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
  fflush(stdout);

  // Registro handler per i segnali
  printf("Registro segnali!\n");
  signals_handler();

  printf("Creo coda richieste...\n");
  Q = create_queue(configuration.maxConnections + 1);

  printf("Creo user manager...\n");  
  user_manager = create_user_manager(configuration.maxHistMsgs, configuration.maxConnections);

  printf("Creo socket!\n");
  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sock_fd < 0) 
  {
    perror("socket");
    return -1;
  }
  
  printf("Bind socket!\n");
  struct sockaddr_un sa;
  strncpy(sa.sun_path, configuration.unixPath, strlen(configuration.unixPath) + 1);
  sa.sun_family = AF_UNIX;
  bind(sock_fd, (struct sockaddr*)&sa, sizeof(sa));

  printf("Listen socket!\n");
  listen(sock_fd, configuration.maxConnections);

  int fd_c, fd_num = 0, fd;
  fd_set rdset, erset;

  if (sock_fd > fd_num) fd_num = sock_fd;
  FD_ZERO(&set);
  FD_SET(sock_fd, &set);


  printf("Creo ThreadPool!\n");
  threadPool = (pthread_t*)malloc(configuration.threadsInPool * sizeof(pthread_t));
  threadArg = (struct worker_arg*)malloc(configuration.threadsInPool *sizeof(struct worker_arg));
  
  for (int i = 0; i < configuration.threadsInPool; i++) 
  {
    threadArg[i].index = i;
    printf("Creo Thread %d!\n", i);
    int ret = pthread_create(&threadPool[i], NULL, worker, (void*)&threadArg[i]);
    if( ret != 0 )
    {
      printf("Errore creazione thread %d\n", i);
    } 
  }

  while (!stopped) {

    pthread_mutex_lock(&mtx_set); 
    rdset = set; 
    erset = set;
    pthread_mutex_unlock(&mtx_set); 

    //printf("Iterazione!\n");

    // TODO add timeout
    struct timeval tv = {0, 10000}; 
    int sel_ret = select(fd_num + 1, &rdset, NULL, &erset, &tv);
    if ( sel_ret < 0 ) {
      printf("Errore select\n");
    } else {
      // printf("Select ret=%d\n", sel_ret);
      for (fd = 0; fd <= fd_num; fd++) {
        if (!FD_ISSET(fd, &rdset) && !FD_ISSET(fd, &erset) ) continue;
        
        if( FD_ISSET(fd, &erset) )
        { 
          printf("Errore FD = %d\n", fd);
        } 
        else if (fd == sock_fd) {

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
        }
      }
    }
  }

  for (int i = 0; i < configuration.threadsInPool; i++) {
    printf("Join thread %d\n", i);
    pthread_join(threadPool[i], NULL);
  }

  // Pulizia finale
  fflush(stdout);
  for (fd = 0; fd <= fd_num; fd++) close(fd);
  close(sock_fd);
  destroy_user_manager(user_manager);
  free(threadPool);
  free(threadArg);
  return 0;
}

void stop_server() {
  printf("KILL\n");
  stopped = 1;
  pthread_cond_broadcast(&(Q->cond));
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
    pthread_mutex_lock(&mtx_stats);
    printStats(fdstat);
    pthread_mutex_unlock(&mtx_stats);
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

  printf("\tTHREAD[%d] Start\n", index);
  message_hdr_t hdr;

  while (!stopped) {

    // TODO condition variable
    printf("\tTHREAD[%d] Aspetto richiesta dalla coda\n",index);
    int fd = pop_queue(Q);
    printf("\tTHREAD[%d] coda = %d\n",index, fd);

    if( stopped == 1 ) break;
    if( fd == -1 ) continue;

    if( readHeader(fd, &hdr) > 0 )
    {
      printf("\tTHREAD[%d] Servo Client [%d]\n", index, fd);
      printf("\tTHREAD[%d] header(op=%d, sender=%s)\n", index, hdr.op, hdr.sender);
      printf("\tTHREAD[%d] Inizio execute\n",index);
      int ret = execute(hdr, fd);
      printf("\tTHREAD[%d] Fine execute (esito=%d)\n",index,ret);

      if( ret == 0 )
      {
        printf("\tTHREAD[%d] Esito positivo, rimetto in coda\n", index);
        pthread_mutex_lock(&mtx_set);  
        FD_SET(fd, &set);
        pthread_mutex_unlock(&mtx_set); 
      }
      else
      {
        printf("\tTHREAD[%d] Esito negativo, non rimetto in coda\n", index);
        disconnect_fd_user(user_manager, fd);
      }
    }
    else
    {
        printf("\tTHREAD[%d] Errore lettura\n", index);
        disconnect_fd_user(user_manager, fd);
    }
 
  }

  printf("\tTHREAD[%d] Stop\n", index);
  return NULL;
}

int execute(message_hdr_t hdr, int client_fd) {
  int tmp;
  message_t received;
  message_t reply;

  received.hdr = hdr;

  op_t operation = hdr.op;
  char *sender = hdr.sender;

  if( sender == NULL || strlen(sender) == 0 )
  {
    setHeader(&(reply.hdr), OP_FAIL, "");        
    sendHeader(client_fd, &(reply.hdr));
    return -1;
  }

  printf("\t\t\tSENDER[%s] OP[%d]\n",sender, operation);

  if( readData(client_fd, &(received.data)) < 0 )
  {
    printf("\t\t\tErrore lettura dati");
    return -1;
  } 

  switch (operation) {
 
    // OK
    // richiesta di registrazione di un ninckname
    case REGISTER_OP: {      
      if( register_user(user_manager, sender) == 0 )
      {   
        tmp = connect_user(user_manager, sender, client_fd);
        if( tmp == -1 ) 
        {
          setHeader(&(reply.hdr), OP_NICK_ALREADY, "");        
          tmp = sendHeader(client_fd, &(reply.hdr));
          if( tmp < 0 ) return -1;
        }   
        else
        {       
          // update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err)
          update_stats(1,0,0,0,0,0,0);
          char *buffer;
          int len = user_list(user_manager, &buffer);
          setHeader(&(reply.hdr), OP_OK, "");      
          setData(&(reply.data), "", buffer, len*(MAX_NAME_LENGTH+1));
          tmp = sendHeader(client_fd, &(reply.hdr));
          if( tmp < 0 ) 
          {
            free(buffer);
            return -1;
          }
          tmp = sendData(client_fd, &(reply.data));
          if( tmp < 0 ) 
          {
            free(buffer);
            return -1;
          }
          free(buffer);
        }
      }
      else
      {
        // update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err)
        update_stats(0,0,0,0,0,0,1);
        setHeader(&(reply.hdr), OP_NICK_ALREADY, "");        
        tmp = sendHeader(client_fd, &(reply.hdr));
        if( tmp < 0 ) return -1;
      }
    } break;

    // OK
    // richiesta di connessione di un client
    case CONNECT_OP: {
      printf("\t\t\tINIZIO OP CONNECT SENDER[%s]\n",sender);
      int tmp = connect_user(user_manager, sender, client_fd);
      printf("\t\t\tESITO CONNECT = %d\n",tmp);
      if( tmp == 0 )
      {
          char *buffer;
          int len = user_list(user_manager, &buffer);
          // update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err)
          update_stats(0,1,0,0,0,0,0);

          setHeader(&(reply.hdr), OP_OK, "");      
          setData(&(reply.data), "", buffer, len*(MAX_NAME_LENGTH+1));
          tmp = sendHeader(client_fd, &(reply.hdr));
          if( tmp < 0 ) 
          {
            free(buffer);
            return -1;
          }
          tmp = sendData(client_fd, &(reply.data));
          if( tmp < 0 ) 
          {
            free(buffer);
            return -1;
          }
          free(buffer);
      }
      else
      {
        // update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err)
        update_stats(0,0,0,0,0,0,1); 
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");        
        tmp = sendHeader(client_fd, &(reply.hdr));
        if( tmp < 0 ) return -1;
      }
      printf("\t\t\tFINE OP CONNECT SENDER[%s]\n",sender);
    } break;

    // richiesta di invio di un messaggio testuale ad un nickname o groupname
    case POSTTXT_OP: {

      printf("\t\t\tINIZIO OP POSTTXT SENDER[%s]\n",sender);

      int len = received.data.hdr.len;
      char *receiver = received.data.hdr.receiver;
      int rec_fd = connected_user(user_manager, receiver);
      printf("\t\t\tFD UTENTE[%s] = %d\n",receiver, rec_fd);

      if( len > configuration.maxMsgSize )
      {
        setHeader(&(reply.hdr), OP_MSG_TOOLONG, "");        
      }
      else if( rec_fd >= 0 ) // registrato e connesso
      {
        // update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err)
        update_stats(0,0,1,0,0,0,0); 
        setHeader(&(reply.hdr), OP_OK, "");  
        message_t *msg = copyMsg(&received);
        msg->hdr.op = TXT_MESSAGE;
        sendRequest(rec_fd, msg);
        free(msg);
      }
      else if( rec_fd == -1 ) // registrato ma non connesso
      {
        // update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err)
        update_stats(0,0,0,1,0,0,0); 
        setHeader(&(reply.hdr), OP_OK, "");  
        message_t *msg = copyMsg(&received);
        post_msg(user_manager, receiver, msg);
        free(msg);
      } 
      else // non registrato
      {
        // update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err)
        update_stats(0,0,0,0,0,0,1); 
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");        
      }

      sendHeader(client_fd, &(reply.hdr));        
        
      printf("\t\t\tFINE OP POSTTXT SENDER[%s]\n",sender);
    } break;

    // richiesta di invio di un messaggio testuale a tutti gli utenti
    case POSTTXTALL_OP: {
      printf("\t\t\tINIZIO OP POSTTXTALL SENDER[%s]\n",sender);
      int *list;
      int pending = post_msg_all(user_manager, &received);
      printf("\t\t\tPENDING = %d\n",pending);
      int sent = fd_list(user_manager, &list);
      printf("\t\t\tSENT = %d\n",sent);
      for(int i=0; i<sent; i++)
      {
        printf("\t\t\tINVIO = %d\n",i);
        message_t *msg = copyMsg(&received);
        msg->hdr.op = TXT_MESSAGE;
        printf("MANDO RICHIESTA (op=%d,from=%s,to=%s,len=%u,buf=%s)\n", msg->hdr.op, msg->hdr.sender, msg->data.hdr.receiver, msg->data.hdr.len, msg->data.buf);
        sendRequest(list[i], msg);
        //free(msg);
      } 
      // free(*list);
      // update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err)
      update_stats(0,0,sent,pending,0,0,0); 
       
      setHeader(&(reply.hdr), OP_OK, "");      // TODO INVIARE AI CONNESSI 
      tmp = sendHeader(client_fd, &(reply.hdr));
      if( tmp < 0 ) 
        return -1;
      printf("\t\t\tFINE OP POSTTXTALL SENDER[%s]\n",sender);
    } break;

    // richiesta di invio di un file ad un nickname o groupname
    case POSTFILE_OP: {
      printf("\t\t\tINIZIO OP POSTFILE SENDER[%s]\n",sender);
      // TODO
      printf("\t\t\tRECEIVED NAME[%s] SIZE[%d]\n", received.data.buf, received.data.hdr.len);
      message_data_t file_data;
      readData(client_fd, &file_data);
      printf("\t\t\tRECEIVED FILESIZE[%d]\n", file_data.hdr.len);
      if( file_data.hdr.len > configuration.maxFileSize*1024 )
      {
        // update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err)
        update_stats(0,0,0,0,0,0,1); 
        setHeader(&(reply.hdr), OP_MSG_TOOLONG, "");      
        tmp = sendHeader(client_fd, &(reply.hdr));
        if( tmp < 0 ) return -1;
      }
      else
      {
        int pathlen = ( strlen(configuration.dirName) + strlen(received.data.buf) + 2 );
        char *filename = malloc( pathlen * sizeof(char*) );
        memset(filename, 0, pathlen );
        strcat(filename, configuration.dirName);
        strcat(filename, "/");
        strcat(filename, received.data.buf);
      
        FILE *fd;
        fd = fopen(filename, "wb");
        if (fd == NULL) {
		      perror("open");
		      fprintf(stderr, "ERRORE: aprendo il file %s\n", filename);
		      fclose(fd);
		      return -1;
	      }
        
        printf("\t\t\tAPERTO %s\n", filename);
          if( fwrite(file_data.buf , sizeof(char), file_data.hdr.len, fd) != file_data.hdr.len ) 
//        if( write(fd, (void*) file_data.buf, file_data.hdr.len) < 0 )  {
        {
		      perror("open");
		      fprintf(stderr, "ERRORE: scrivendo il file %s\n", filename);
		      fclose(fd);
		      return -1;
	      }

        fclose(fd);

        char *receiver = received.data.hdr.receiver;
        int rec_fd = connected_user(user_manager, receiver);
        
        if( rec_fd >= 0 ) // registrato e connesso
        {
          setHeader(&(reply.hdr), OP_OK, "");  
          message_t *msg = copyMsg(&received);
          msg->hdr.op = FILE_MESSAGE;
          sendRequest(rec_fd, msg);
          free(msg);
        }
        else if( rec_fd == -1 ) // registrato ma non connesso
        {
          setHeader(&(reply.hdr), OP_OK, "");  
          message_t *msg = copyMsg(&received);
          post_msg(user_manager, receiver, msg);
          free(msg);
        } 
        else // non registrato
        {
          setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");        
        }
      
        tmp = sendHeader(client_fd, &(reply.hdr));
        if( tmp < 0 ) return -1;
      }
      printf("\t\tFINE OP POSTFILE SENDER[%s]\n",sender);
    } break;

    // richiesta di recupero di un file
    case GETFILE_OP: {
      printf("\t\t\tINIZIO OP GETFILE SENDER[%s]\n",sender);

      int pathlen = ( strlen(configuration.dirName) + strlen(received.data.buf) + 2 );
      char *filename = malloc( pathlen * sizeof(char*) );
      memset(filename, 0, pathlen );
      strcat(filename, configuration.dirName);
      strcat(filename, "/");
      strcat(filename, received.data.buf);

      char *mappedfile;
      int fd = open(filename, O_RDONLY);
      if (fd<0) {
        setHeader(&(reply.hdr), OP_NO_SUCH_FILE, "");      
        tmp = sendHeader(client_fd, &(reply.hdr));
        if( tmp < 0 ) return -1;
      }
      else
      {
	      struct stat st;
	      if (stat(filename, &st)==-1 ||  !S_ISREG(st.st_mode) ) 
        {
          setHeader(&(reply.hdr), OP_NO_SUCH_FILE, "");      
          tmp = sendHeader(client_fd, &(reply.hdr));
          if( tmp < 0 ) return -1;
	      }

	      int filesize = st.st_size; 

        mappedfile = mmap(NULL, filesize, PROT_READ,MAP_PRIVATE, fd, 0);
        if (mappedfile == MAP_FAILED) {
          setHeader(&(reply.hdr), OP_NO_SUCH_FILE, "");      
          tmp = sendHeader(client_fd, &(reply.hdr));
          if( tmp < 0 ) return -1;
        }
        else
        {
          close(fd);
          setHeader(&(reply.hdr), OP_OK, "");      
          tmp = sendHeader(client_fd, &(reply.hdr));
          if( tmp < 0 ) return -1;   
          setData(&(reply.data), "", mappedfile, filesize);          
          tmp = sendData(client_fd, &(reply.data));
          if( tmp < 0 ) return -1;
        }
       }
       printf("\t\t\tFINE OP GETFILE SENDER[%s]\n",sender);
    } break;

    // richiesta di recupero della history dei messaggi
    case GETPREVMSGS_OP: {
      printf("\t\t\tINIZIO OP GETPREVMSGS SENDER[%s]\n",sender);
      message_t **list;
      int len = retrieve_user_msg(user_manager, sender, &list);
      printf("\t\t\tGETPREVSMGS len=%d\n",len);
      if( len >= 0 )
      {        
        setHeader(&(reply.hdr), OP_OK, "");
        setData(&(reply.data), "", (char*)&len, sizeof(size_t));
        sendRequest(client_fd, &reply);
        for(int i=0; i<len; i++) sendRequest(client_fd, list[i]);
//        for(int i=0; i<len; i++){
//          free(list[i]->data.buf); 
//          free(list[i]); 
//        }
//        free(*list);      
      } 
      else
      {
        setHeader(&(reply.hdr), OP_FAIL, "");
        sendHeader( client_fd, &(reply.hdr) );
      } 
      printf("\t\t\tFINE OP GETPREVMSGS SENDER[%s]\n",sender);
    } break;

    // OK
    // richiesta di avere la lista di tutti gli utenti attualmente connessi
    case USRLIST_OP: {
      printf("\t\t\tINIZIO OP USRLIST SENDER[%s]\n",sender);
      char *buffer;
      int len = user_list(user_manager, &buffer);
      setHeader(&(reply.hdr), OP_OK, "");      
      setData(&(reply.data), "", buffer, len*(MAX_NAME_LENGTH+1));
      tmp = sendHeader(client_fd, &(reply.hdr));
      if( tmp < 0 ) 
      {
        free(buffer);
        return -1;
      }
      tmp = sendData(client_fd, &(reply.data));
      if( tmp < 0 ) 
      {
        free(buffer);
        return -1;
      }
      free(buffer);
      printf("\t\t\tFINE OP USRLIST SENDER[%s]\n",sender);
    } break;

    // OK
    // richiesta di deregistrazione di un nickname o groupname
    case UNREGISTER_OP: {    
      printf("\t\t\tINIZIO OP UNREGISTER SENDER[%s]\n",sender); 
      if( unregister_user(user_manager, sender) == 0 )
        setHeader(&(reply.hdr), OP_OK, "");      
      else
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");        
      
      sendHeader(client_fd, &(reply.hdr));
      printf("\t\t\tFINE OP UNREGISTER SENDER[%s]\n",sender);
    } break;

    // OK
    // richiesta di disconnessione
    case DISCONNECT_OP: {
      printf("\t\t\tINIZIO OP DISCONNECT SENDER[%s]\n",sender);
      if( disconnect_user(user_manager, sender) == 0 )
        setHeader(&(reply.hdr), OP_OK, "");      
      else
        setHeader(&(reply.hdr), OP_NICK_UNKNOWN, "");        
      
      sendHeader(client_fd, &(reply.hdr));
      printf("\t\t\tFINE OP DISCONNECT SENDER[%s]\n",sender);
    } break;

    // richiesta di creazione di un gruppo
    case CREATEGROUP_OP: {
      printf("\t\t\tINIZIO OP CREATEGROUP SENDER[%s]\n",sender);
      // TODO
      printf("\t\t\tFINE OP CREATEGROUP SENDER[%s]\n",sender);
    } break;

    // richiesta di aggiunta ad un gruppo
    case ADDGROUP_OP: {
      printf("\t\t\tINIZIO OP ADDGROUP SENDER[%s]\n",sender);
      // TODO
      printf("\t\t\tFINE OP ADDGROUP SENDER[%s]\n",sender);
    } break;

    // richiesta di rimozione da un gruppo
    case DELGROUP_OP: {
      printf("\t\t\tINIZIO OP DELGROUP SENDER[%s]\n",sender);
      // TODO
      printf("\t\t\tFINE OP DELGROUP SENDER[%s]\n",sender);
    } break;

    default: {
    
    } break;
  }

  printf("\t\t\tEND SENDER[%s] OP[%d]\n",sender, operation);
  return 0;
}


void update_stats(int reg, int on, int msg_del, int msg_pen, int file_del, int file_pen, int err)
{
  pthread_mutex_lock(&mtx_stats);
  chattyStats.nusers = reg; 
	chattyStats.nonline = on;
	chattyStats.ndelivered = msg_del;
	chattyStats.nnotdelivered = msg_pen;
	chattyStats.nfiledelivered = file_del;
	chattyStats.nfilenotdelivered = file_pen;
	chattyStats.nerrors = err;
  pthread_mutex_unlock(&mtx_stats);
}
