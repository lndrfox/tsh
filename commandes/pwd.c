#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include "print.h"

int main(void){

	char bufdir [PATH_MAX + 1];
	getcwd(bufdir,sizeof(bufdir));
	char * entry=malloc(sizeof(bufdir)+sizeof(getenv("tar"))+sizeof("/"));

	if(entry==NULL){

		perror("malloc");
		exit(-1);
	}

	memset(entry,0,sizeof(bufdir)+sizeof(getenv("tar")));
	entry= strcat(entry,bufdir);
	entry=strcat(entry,"/");
	entry = strcat(entry,getenv("tar"));
	prints(entry);
	prints("\n");
	free(entry);
	return(0);
}