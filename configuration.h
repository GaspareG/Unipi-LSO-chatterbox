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
 * @file configuration.h
 * @brief Dichiarazioni delle strutture e funzioni necessarie
 *        per le configurazioni del server
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

/**
 *  @struct serverConfiguration
 *  @brief configurazioni del server
 *
 *  @var   maxConnections  numero messaggio delle connessioni contemporanee
 *  @var   threadsInPool   numero dei thread nella threadpool
 *  @var   maxMsgSize      dimensione massima di un messaggio
 *  @var   maxFileSize     dimensione massima (in kb) di un file
 *  @var   maxHistMsgs     numero massimo dei messaggi da ricordare
 *  @var   unixPath        path del socket
 *  @var   dirName         directory dove salvare i file scambiati
 *  @var   statFileName    file dove appendere le statistiche
 */
struct serverConfiguration {
  unsigned long maxConnections;
  unsigned long threadsInPool;
  unsigned long maxMsgSize;
  unsigned long maxFileSize;
  unsigned long maxHistMsgs;
  char *unixPath;
  char *dirName;
  char *statFileName;
};

/**
 * @function readConfig
 * @brief effettua il parsing del file di configurazione
 *
 * @param fileName nome del file di configurazione da analizzare
 * @param config puntatore alla struttura dove scrivere le configurazioni
 * @return   0 se i dati sono stati letti correttamente 
 *          -1 per errore
 */
int readConfig(char *fileName, struct serverConfiguration *config);

#endif /* CONFIGURATION_H_ */
