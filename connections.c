/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
#ifndef CONNECTIONS_C_
#define CONNECTIONS_C_

#define MAX_RETRIES     10
#define MAX_SLEEPING     3
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX  64
#endif

#include <message.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>  
#include <unistd.h>      
#include <stdlib.h>
#include <connections.h>
#include <stdio.h>
#include <config.h>

int openConnection(char* path, unsigned int ntimes, unsigned int secs)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    
    if( fd == -1 ) return -1;
    
    struct sockaddr_un sa;
    strncpy(sa.sun_path, path, UNIX_PATH_MAX);
    sa.sun_family=AF_UNIX;

    if( ntimes > MAX_RETRIES ) ntimes = MAX_RETRIES;
    if( secs > MAX_SLEEPING ) secs = MAX_SLEEPING;

    while( ntimes-- && connect(fd, (struct sockaddr*) &sa, sizeof(sa) ) == -1 )
    {
        if ( errno == ENOENT ) sleep(secs); 
        else return -1;
    }

    return fd;
}

int readBuffer(long connfd, char *buffer, unsigned int length)
{   
    char *tmp = buffer;
    while( length > 0 )
    {
        int red = read(connfd, tmp, length);
	if( red < 0 ) return -1;
        tmp += red;
        length -= red;
    }
    return 1;
}

int readHeader(long connfd, message_hdr_t *hdr)
{
    memset(hdr, 0, sizeof(message_hdr_t));
    int tmp = read(connfd, hdr, sizeof(message_hdr_t));
    //printf("\t\t\t\t\t[%ld] readHeader(op=%d) tmp=%d\n",connfd, hdr->op, tmp);
    //printf("\t\t\t\t\t[%ld] readHeader(sender=%s) tmp=%d\n",connfd, hdr->sender, tmp);
    if( tmp < 0 ) return -1;
    //int tmp = read(connfd, &(hdr->op), sizeof(op_t));
    //if( tmp < 0 ) return -1;
    //tmp = readBuffer(connfd, hdr->sender, MAX_NAME_LENGTH+1); 
    //if( tmp < 0 ) return -1;
    return 1;
}

int readData(long connfd, message_data_t *data)
{
    memset(data, 0, sizeof(message_data_t));
    int tmp = read(connfd, &(data->hdr), sizeof(message_data_hdr_t));
    //printf("\t\t\t\t\t[%ld] readHeader(len=%d) tmp=%d\n",connfd, data->hdr.len, tmp);
    //printf("\t\t\t\t\t[%ld] readHeader(receiver=%s) tmp=%d\n",connfd, data->hdr.receiver, tmp);
    if( tmp < 0 ) return -1;
    //int tmp = read(connfd, &(data->hdr.len), sizeof(unsigned int));
    //if( tmp < 0 ) return -1;
    //tmp = readBuffer(connfd, data->hdr.receiver, MAX_NAME_LENGTH+1);
    //if( tmp < 0 ) return -1;
    data->buf = (char*) malloc( data->hdr.len * sizeof(char) );
    tmp = readBuffer(connfd, data->buf, data->hdr.len);
    if( tmp < 0 ) return -1;
    return 1;
}


int readMsg(long connfd, message_t *msg)
{
    memset(msg, 0, sizeof(message_t));
    int tmp = readHeader(connfd, &(msg->hdr)); 
    if( tmp < 0 ) return -1;
    tmp = readData(connfd, &(msg->data));
    if( tmp < 0 ) return -1;
    return 1;
}

int sendBuffer(long connfd, char *buffer, unsigned int length)
{
    while( length > 0 )
    {
    	//printf("\t\t\t\t\tsendBuffer(length=%u)\n",length);
        int wrote = write(connfd, buffer, length);
    	//printf("\t\t\t\t\tsendBuffer(wrote=%u)\n",wrote);
	if( wrote < 0 ) return -1;
        buffer += wrote;
        length -= wrote;
    }
    return 1;
}

int sendRequest(long connfd, message_t *msg)
{
    int tmp = sendHeader(connfd, &(msg->hdr));
    if( tmp < 0 ) return -1;
    tmp = sendData(connfd, &(msg->data));
    if( tmp < 0 ) return -1;
    return 1;
}

int sendData(long connfd, message_data_t *data)
{
    int tmp = write(connfd, &(data->hdr), sizeof(message_data_hdr_t));
    //printf("\t\t\t\t\tsendDataHeader tmp=%d\n", tmp);
    if( tmp < 0 ) return -1;
    //int tmp = write(connfd, &(data->hdr.len), sizeof(unsigned int));
    //if( tmp < 0 ) return -1;
    //tmp = sendBuffer(connfd, data->hdr.receiver, MAX_NAME_LENGTH+1);
    //if( tmp < 0 ) return -1;
    tmp = sendBuffer(connfd, data->buf, data->hdr.len);
    if( tmp < 0 ) return -1;
    return 1;
}

int sendHeader(long connfd, message_hdr_t *msg)
{
    int tmp = write(connfd, msg, sizeof(message_hdr_t));
    if( tmp < 0 ) return -1;
    //int tmp = write(connfd, &(msg->op), sizeof(op_t));
    //if( tmp < 0 ) return -1;
    //printf("\t\t\t\t\tsendHeader(op=%d) tmp=%d\n",msg->op, tmp);
    //tmp = sendBuffer(connfd, msg->sender, MAX_NAME_LENGTH+1);  
    //printf("\t\t\t\t\tsendHeader(sender=%s) tmp=%d\n",msg->sender, tmp);
    //if( tmp < 0 ) return -1;
    return 1;
}


#endif /* CONNECTIONS_C_ */
