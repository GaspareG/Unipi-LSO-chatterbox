/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file stats.h
 * @brief Definizione della struttura e funzione per la gestione delle
 *        statistiche raccolte dal server
 *
 * @author Gaspare Ferraro 520549 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 * originale dell'autore  
 */

#ifndef STATS_H__
#define STATS_H_

#include <stdio.h>
#include <time.h>

struct statistics {
  unsigned long nusers;         // n. di utenti registrati
  unsigned long nonline;        // n. di utenti connessi
  unsigned long ndelivered;     // n. di messaggi testuali consegnati
  unsigned long nnotdelivered;  // n. di messaggi testuali non ancora consegnati
  unsigned long nfiledelivered;     // n. di file consegnati
  unsigned long nfilenotdelivered;  // n. di file non ancora consegnati
  unsigned long nerrors;            // n. di messaggi di errore
};

/* aggiungere qui altre funzioni di utilita' per le statistiche */

/**
 * @function printStats
 * @brief Stampa le statistiche nel file passato come argomento
 *
 * @param fout descrittore del file aperto in append.
 *
 * @return 0 in caso di successo, -1 in caso di fallimento
 */
static inline int printStats(FILE *fout) {
  extern struct statistics chattyStats;

  if (fprintf(fout, "%ld - %ld %ld %ld %ld %ld %ld %ld\n",
              (unsigned long)time(NULL), chattyStats.nusers,
              chattyStats.nonline, chattyStats.ndelivered,
              chattyStats.nnotdelivered, chattyStats.nfiledelivered,
              chattyStats.nfilenotdelivered, chattyStats.nerrors) < 0)
    return -1;
  fflush(fout);
  return 0;
}

#endif /* STATS_H_ */
