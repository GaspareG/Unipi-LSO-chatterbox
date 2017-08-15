#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "configuration.h"

int readConfig(char *fileName, struct serverConfiguration *config)
{
	FILE *fd;
	fd = fopen(fileName, "r");

	fseek(fd, 0L, SEEK_END);
	int fileSize = ftell(fd);
	fseek(fd,0,SEEK_SET);
	
	char *currLine = (char*) malloc( fileSize*sizeof(char) );
	char *key = (char*) malloc( fileSize*sizeof(char) );
	char *value = (char*) malloc( fileSize*sizeof(char) );

	while( fgets(currLine, fileSize, fd) != NULL )
	{
		int lineSize = strlen(currLine);		
		if( lineSize <= 1 || currLine[0] == '#' ) continue;	
	
		sscanf(currLine,"%s = %s",key, value);

		int keySize = strlen(key);
		int valueSize = strlen(value);
		if( keySize == 0 || valueSize == 0 ) continue;

		if( strcmp(key,"UnixPath") == 0 ) 
		{
			config->unixPath = (char*) malloc( (valueSize+1) * sizeof(char) );		
			strncpy(config->unixPath, value, valueSize+1);		
		}
		else if( strcmp(key,"DirName") == 0 ) 
		{
			config->dirName = (char*) malloc( (valueSize+1) * sizeof(char) );		
			strncpy(config->dirName, value, valueSize+1);				
		}
		else if( strcmp(key,"StatFileName") == 0 ) 
		{
			config->statFileName = (char*) malloc( (valueSize+1) * sizeof(char) );	
			strncpy(config->statFileName, value, valueSize+1);		
		}
		else if( strcmp(key,"MaxConnections") == 0 ) 
		{
			config->maxConnections = strtoul(value, NULL, 10);	
		}
		else if( strcmp(key,"ThreadsInPool") == 0 ) 
		{
			config->threadsInPool = strtoul(value, NULL, 10);				
		}
		else if( strcmp(key,"MaxMsgSize") == 0 ) 
		{
			config->maxMsgSize = strtoul(value, NULL, 10);				
		}
		else if( strcmp(key,"MaxFileSize") == 0 ) 
		{
			config->maxFileSize = strtoul(value, NULL, 10);				
		}
		else if( strcmp(key,"MaxHistMsgs") == 0 ) 
		{
			config->maxHistMsgs = strtoul(value, NULL, 10);				
		}

		fflush(stdout);	
	}

	free(currLine);
	free(key);
	free(value);

	fclose(fd);
	fflush(stdout);
	return 0;
}
