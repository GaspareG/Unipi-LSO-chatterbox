/*
 * chatterbox Progetto del corso di LSO 2017 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 * Studente: Francesco Vatteroni
 */
#ifndef CONNECTIONS_C_
#define CONNECTIONS_C_

#ifndef MAX_RETRIES
#define MAX_RETRIES     10
#endif

#ifndef MAX_SLEEPING
#define MAX_SLEEPING     3
#endif

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX  64
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif


#include "message.h"

#include "connections.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <sys/un.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <config.h>


int openConnection(char* path, unsigned int ntimes, unsigned int secs);//fatto

// -------- server side ----- 
int readHeader(long connfd, message_hdr_t *hdr);
int readData(long fd, message_data_t *data);
int readMsg(long fd, message_t *msg);

// ------- client side ------
int sendRequest(long fd, message_t *msg);
int sendData(long fd, message_data_t *msg);
int sendHeader(long fd, message_hdr_t *hdr);

// ---------- aux -----------
int writeaux(long fd, char * buf, int size);
int readaux(long fd, char * buf, int size);

int openConnection(char* path, unsigned int ntimes, unsigned int secs)
{
	int fds;
	struct sockaddr_un sa;
	strncpy(sa.sun_path, path, UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;
	fds=socket(AF_UNIX,SOCK_STREAM,0);
	int i=0;
	while (i<ntimes)
	{
		connect(fds,(struct sockaddr*) &sa, sizeof(sa));
		if(errno==ENOENT){i++; sleep(secs);}
		else
		{
			return fds;
		}
	}
	return -1;
}

int sendRequest(long fd, message_t *msg)
{
	int aux=0;
	aux = sendHeader(fd, &msg->hdr);
	if(aux!=0) { /*printf("A:%ld %d\n",fd, errno);*/ return -1;}
	aux = sendData(fd, &msg->data);
	if(aux!=0) { /*printf("B:%ld %d\n",fd, errno);*/ return -1;}
	return 0;
}

int sendData(long fd, message_data_t *msg)
{
	int aux=0;
	aux = writeaux (fd, msg->hdr.receiver, sizeof(char)*(MAX_NAME_LENGTH+1));
	if(aux<0) { /*printf("%ld %d\n",fd, errno); */return -1;}
	aux = write (fd, &msg->hdr.len, sizeof(int));
	if(aux<0) { /*printf("%ld %d\n",fd, errno); */return -1;}
	
	if(msg->hdr.len>0)
	{
		aux = writeaux (fd, msg->buf, sizeof(char)*(msg->hdr.len));
		if(aux<0) { /*printf("%ld %d\n",fd, errno); */return -1;}
	}
	return 0;
}


int sendHeader(long fd, message_hdr_t *msg)
{
	int aux=0;
	aux = write (fd, &(msg->op), sizeof(op_t));
	if(aux<0) { /*printf("%ld %d\n",fd, errno); */return -1;}
	aux = writeaux (fd, msg->sender, sizeof(char)*(MAX_NAME_LENGTH+1));
	if(aux<0) { /*printf("%ld %d\n",fd, errno); */return -1;}
	return 0;
}

int readHeader(long connfd, message_hdr_t *hdr)
{
	int aux=0;
	memset(hdr, 0, sizeof(message_hdr_t));
	aux = read (connfd, &(hdr->op), sizeof(op_t));
	if(aux<0) { /*printf("%ld %d\n",connfd, errno); */return -1;}
	aux = readaux (connfd, hdr->sender, sizeof(char)*(MAX_NAME_LENGTH+1));
	if(aux<0) { /*printf("%ld %d\n",connfd, errno); */return -1;}
	return 1;
}

int readData(long fd, message_data_t *data)
{
	int aux=0;
	memset(data, 0, sizeof(message_data_t));
	aux = readaux (fd, data->hdr.receiver, sizeof(char)*(MAX_NAME_LENGTH+1));
	if(aux<0) { /*printf("%ld %d\n",fd, errno); */return -1;}
	aux = read (fd, &data->hdr.len, sizeof(int));
	if(aux<0) { /*printf("%ld %d\n",fd, errno); */return -1;};
	
	
	if(data->hdr.len>0)	
	{
		data->buf = (char*) malloc(sizeof(char)*(data->hdr.len));
		memset(data->buf, 0, sizeof(char)*(data->hdr.len));
		aux = readaux (fd, data->buf, sizeof(char)*(data->hdr.len));
		if(aux<0) { /*printf("%ld %d\n",fd, errno); */return -1;};
	}
	else data->buf = NULL;
	
	return 1;
}

int readMsg(long fd, message_t *msg)
{
	int aux=0;

	op_t op = OP_END;
	char R[MAX_NAME_LENGTH];
	memset(R, 0, (sizeof(char)*(MAX_NAME_LENGTH+1)));
	strncpy(R, "Ricezione", 9);
	
	aux = writeaux (fd, (char*)&op, sizeof(op_t));
	if(aux<0) { /*printf("%ld %d\n",fd, errno);*/ return -1;}
	aux = writeaux (fd, R, sizeof(char)*(MAX_NAME_LENGTH+1));
	if(aux<0) { /*printf("%ld %d\n",fd, errno);*/ return -1;}

	aux = readHeader(fd, &msg->hdr);
	if(aux<0) { /*printf("%ld %d\n",fd, errno);*/ return -1;}
	aux = readData(fd, &msg->data);
	if(aux<0) { /*printf("%ld %d\n",fd, errno);*/ return -1;}
	return 1;
}

int writeaux(long fd, char * buf, int size)
{
	int sizeaux = size;
	int rc=0;
	char *bufaux = NULL;
	bufaux = buf;

	while(sizeaux>0) 
	{
		rc = write((int)fd, bufaux, sizeaux);
		if (rc == -1)
		{
			if (errno != EINTR) { /*printf("%ld %d\n",fd, errno);*/ return -1;}
		}
		if (rc == 0) return 0;
		sizeaux = sizeaux-rc;
		bufaux  = bufaux+rc;
	}
	//printf("sto scrivendo: %s\n", buf);
	return size;
}

int readaux(long fd, char * buf, int size)
{
	int sizeaux = size;
	int rc=0;
	char *bufaux = NULL;
	bufaux = buf;

	while(sizeaux>0) 
	{
		rc = read((int)fd, bufaux, sizeaux);
		if (rc == -1)
		{
			if (errno != EINTR) { /*printf("%ld %d\n",fd, errno);*/ return -1;}
		}
		if (rc == 0) return 0;
		sizeaux = sizeaux-rc; 
		bufaux  = bufaux+rc;
	}
	//printf("sto leggendo: %s\n", buf);
	return size;
}
#endif
