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
#include "config.h"
#include "configuration.h"
#include "stats.h"

/* struttura che memorizza le statistiche del server, struct statistics
 * e' definita in stats.h.
 *
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0 };
struct serverConfiguration configuration;

static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

// Segnatura delle funzioni utilizzate
void signalsHandler();
void stopServer();
void printStatistics();

int main(int argc, char *argv[]) {

  // Controllo uso corretto degli qrgomenti
  if( argc < 3 || strncmp(argv[1],"-f",2) != 0  )
  {
      usage(argv[0]);
      return 0;
  }

  // Registro handler per i segnali
  signalsHandler();

  // Parso il file di configurazione
  readConfig(argv[2], &configuration);


  return 0;
}

void signalsHandler()
{
	struct sigaction exitHandler; 
	struct sigaction statsHandler;

	memset( &exitHandler, 0, sizeof(exitHandler) );
	memset( &statsHandler, 0, sizeof(statsHandler) );

	exitHandler.sa_handler = stopServer;
	statsHandler.sa_handler = printStatistics;
	
	sigaction(SIGINT,&exitHandler,NULL);
	sigaction(SIGQUIT,&exitHandler,NULL);
	sigaction(SIGTERM,&exitHandler,NULL);
	sigaction(SIGUSR1,&statsHandler,NULL);
	
	signal(SIGPIPE, SIG_IGN);
}


void stopServer()
{
	printf("KILL");
}

void printStatistics()
{
	printf("STATS");
}
