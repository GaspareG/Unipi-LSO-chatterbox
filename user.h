 /*
 * chatterbox Progetto del corso di LSO 2017 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

#ifndef USER_H_
#define USER_H_

#include <config.h>

struct user {
	char name[MAX_NAME_LENGTH+1];
};
    
int registerUser();
int connectUser();

int userList();

int unregisterUser();
int disconnectUser();
#endif /* USER_H_ */
