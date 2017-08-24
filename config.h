/*
 * membox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 */

#if !defined(CONFIG_H_)
#define CONFIG_H_

#define MAX_NAME_LENGTH                  32

/* aggiungere altre define qui */

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX  108
#endif

#define ec_val_ret(v,r,s) \
	if( (s) == (v) ) return (r);

// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */
