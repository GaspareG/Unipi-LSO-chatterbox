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
#include <sys/socket.h>
#include <sys/un.h> 
#include "config.h"
#include "configuration.h"
#include "stats.h"

/* struttura che memorizza le statistiche del server, struct statistics
 * e' definita in stats.h.
 *
 */
struct statistics  chattyStats = {0, 0, 0, 0, 0, 0, 0};
struct serverConfiguration configuration = {0, 0, 0, 0, 0, NULL, NULL, NULL};

static void usage(const char *progname) {
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

// Segnatura delle funzioni utilizzate
void signalsHandler();
void stopServer();
void printStatistics();
void *worker(void *arg);

int stopped ;

pthread_t *threadPool;
struct worker_arg *threadArg;

int main(int argc, char *argv[]) {

    // Controllo uso corretto degli qrgomenti
    if( argc < 3 || strncmp(argv[1],"-f",2) != 0  )
    {
        usage(argv[0]);
        return 0;
    }

    // Parso il file di configurazione
    printf("Leggo configurazioni...\n");
    readConfig(argv[2], &configuration);

    // Stampo le configurazioni lette
    printf("Configurazioni lette:\n");
    if( configuration.statFileName != NULL )
        printf("%s -> [%s]\n","statFileName", configuration.statFileName);
    if( configuration.dirName != NULL )
        printf("%s -> [%s]\n","dirName", configuration.dirName);
    if( configuration.unixPath != NULL )
        printf("%s -> [%s]\n","unixPath", configuration.unixPath);

    printf("%s -> [%lu]\n","maxHistMsgs", configuration.maxHistMsgs);
    printf("%s -> [%lu]\n","maxFileSize", configuration.maxFileSize);
    printf("%s -> [%lu]\n","maxMsgSize", configuration.maxMsgSize);
    printf("%s -> [%lu]\n","threadsInPool", configuration.threadsInPool);
    printf("%s -> [%lu]\n","maxConnections", configuration.maxConnections);
    fflush(stdout);

    // Registro handler per i segnali
    printf("Registro segnali!\n");
    signalsHandler();
    printf("Segnali registrati!\n");

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0); 

    if (sock_fd < 0)
    {
        perror("socket");
        return -1;
    }

    // Creo i Thread
    threadPool = (pthread_t*) malloc( configuration.threadsInPool* sizeof(pthread_t) ); 
    threadArg = (struct worker_arg*) malloc( configuration.threadsInPool * sizeof(struct worker_arg) );

    for(int i=0; i < configuration.threadsInPool; i++)
    {
        threadArg[i].index = i;
                   
        int ret = pthread_create(&threadPool[i], NULL, worker, (void *) &threadArg[i]);
        if( ret != 0 )
        {

        }
    }

    struct sockaddr_un sa;
    strncpy(sa.sun_path, configuration.unixPath, strlen(configuration.unixPath)+1);
    sa.sun_family=AF_UNIX;
    bind(sock_fd, (struct sockaddr *) &sa, sizeof(sa));

    listen(sock_fd, configuration.maxConnections);

    while( !stopped ){

    }   

    for(int i=0; i < configuration.threadsInPool; i++)
    {
        pthread_join( threadPool[i], NULL);
    }   
    free(threadPool);
    free(threadArg);
    return 0;
}

void stopServer()
{
    printf("KILL\n");
    stopped = 1;
    fflush(stdout);
}

void printStatistics()
{
    printf("STATS\n");
    FILE *fdstat;
    fdstat = fopen(configuration.statFileName,"a");
    if( fdstat != NULL )
    {
        chattyStats.nusers += 1;
        printStats(fdstat);
    }
    fclose(fdstat);
    fflush(stdout);
}

void signalsHandler()
{
    struct sigaction exitHandler; 
    struct sigaction statsHandler;

    memset( &exitHandler, 0, sizeof(exitHandler) );
    memset( &statsHandler, 0, sizeof(statsHandler) );

    stopped = 0;
    exitHandler.sa_handler = stopServer;
    statsHandler.sa_handler = printStatistics;
    
    sigaction(SIGINT,&exitHandler,NULL);
    sigaction(SIGQUIT,&exitHandler,NULL);
    sigaction(SIGTERM,&exitHandler,NULL);

    if( configuration.statFileName != NULL && strlen(configuration.statFileName) > 0 )
        sigaction(SIGUSR1,&statsHandler,NULL);

    // TODO Check error    
    signal(SIGPIPE, SIG_IGN);
}

void * worker(void *arg)
{
    int index = ((struct worker_arg*) arg)->index;
    struct worker_t data = ((struct worker_arg*) arg)->data;

    data.empty = 0 ;

    printf("THREAD %d START %d\n",index, data.empty);
    while( !stopped );

    // TODO QUALCOSA
    printf("THREAD %d STOP\n",index);

    return NULL;
}
