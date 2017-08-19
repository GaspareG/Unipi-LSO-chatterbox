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

int openConnection(char* path, unsigned int ntimes, unsigned int secs)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
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
    while( length )
    {
        int red = read(connfd, buffer, sizeof(char)*length);
        buffer += red;
        length -= red;
    }
    return 1;
}

int readHeader(long connfd, message_hdr_t *hdr)
{
    memset(hdr, 0, sizeof(message_hdr_t));
    read(connfd, &(hdr->op), sizeof(op_t));
    readBuffer(connfd, hdr->sender, MAX_NAME_LENGTH+1);    
    return 1;
}

int readData(long connfd, message_data_t *data)
{
    read(connfd, &(data->hdr.len), sizeof(unsigned int));
    readBuffer(connfd, data->hdr.receiver, MAX_NAME_LENGTH+1);
    data->buf = (char*) malloc( data->hdr.len * sizeof(char) );
    readBuffer(connfd, data->buf, data->hdr.len);
    return 1;
}


int readMsg(long connfd, message_t *msg)
{
    memset(msg, 0, sizeof(message_t));
    readHeader(connfd, &(msg->hdr)); 
    readData(connfd, &(msg->data));
    return 1;
}

int sendBuffer(long connfd, char *buffer, unsigned int length)
{
    while( length )
    {
        int wrote = write(connfd, buffer, sizeof(char)*length);
        buffer += wrote;
        length -= wrote;
    }
    return 1;
}

int sendRequest(long connfd, message_t *msg)
{
    sendHeader(connfd, &(msg->hdr));
    sendData(connfd, &(msg->data));
    return 1;
}

int sendData(long connfd, message_data_t *data)
{
    write(connfd, &(data->hdr.len), sizeof(unsigned int));
    sendBuffer(connfd, data->hdr.receiver, MAX_NAME_LENGTH+1);
    sendBuffer(connfd, data->buf, data->hdr.len);
    return 1;
}

int sendHeader(long connfd, message_hdr_t *msg)
{
    write(connfd, &(msg->op), sizeof(op_t));
    sendBuffer(connfd, msg->sender, MAX_NAME_LENGTH+1);  
    return 1;
}


#endif /* CONNECTIONS_C_ */
