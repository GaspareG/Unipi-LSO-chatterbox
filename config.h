/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file config.h
 * @brief Definizione configurazioni di base
 *
 * @author Gaspare Ferraro 520549 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 * originale dell'autore  
 */


#ifndef CONFIG_H_
#define CONFIG_H_

#define MAX_NAME_LENGTH 32

/* aggiungere altre define qui */

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX  108
#endif

// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */
