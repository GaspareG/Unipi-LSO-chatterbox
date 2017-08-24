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

int readConfig(char *fileName, struct serverConfiguration *config);

#endif /* CONFIGURATION_H_ */
