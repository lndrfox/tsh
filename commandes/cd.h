#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/*#define _BSD_SOURCE 
#define _POSIX_C_SOURCE >= 200112L 
#define _XOPEN_SOURCE >= 600 */

void cd (char * p){


	char * path = malloc(strlen(p)+strlen(getenv("tar"))+1);
	memset(path,0,strlen(path));

	path = strcat(path,getenv("tar"));
	path=strcat(path,"/");
	path=strcat(path,p);

	path = path_is_valid(path);

	if(path==NULL){

		printf("ERREUR TAMER");
	}

	else{

		setenv("tar",path,1);
	}

	

}