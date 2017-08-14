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

#if !defined(CONFIGURATION_H_)
#define CONFIGURATION_H_ 


struct serverConfiguration {
	unsigned long maxConnections;
	unsigned long threadsInPool;
	unsigned long maxMsgSize;
	unsigned long maxFileSize;
	unsigned long maxhistMsgs;
	char *unixPath;
	char *dirName;
	char *statFileName;
};

int readConfig(char *fileName, struct serverConfiguration *config);


#endif /* MEMBOX_STATS_ */
