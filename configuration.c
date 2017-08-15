
#include <stdio.h>
#include <stdlib.h>


int readConfig(char *fileName, struct serverConfiguration *config)
{
	printf("READ CONFIG\n");
	FILE *fd;
	fd = fopen(fileName, "r");

	fseek(fd, 0L, SEEK_END);
	int size=ftell(fd);
	fseek(fd,0,SEEK_SET);
	
	printf("FILE %d byte\n", size);
	char *currline = (char*) malloc( size*sizeof(char) );

	while( fgets(currline, size, fd) != NULL )
	{
		int lineSize = strlen(currline);
		if( lineSize == 0 ) continue ;
		
		if( currline[0] == '#') continue;		
		char *key = (char*) malloc( lineSize*sizeof(char) );
		char *value = (char*) malloc( lineSize*sizeof(char) );
		sscanf(currline,"%s = %s",key, value);
		printf("[%d] [%s] = [%s]\n",lineSize,key,value);
		fflush(stdout);	
	}

	
	fclose(fd);
	fflush(stdout);
}
