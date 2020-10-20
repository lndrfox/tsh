#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "tar.h"
#include "tar_nav.h"

/*#define _BSD_SOURCE 
#define _POSIX_C_SOURCE >= 200112L 
#define _XOPEN_SOURCE >= 600 */

void append_path(char * path){

	size_t size=strlen(getenv("tar"))+strlen(path);
	char * cat1 = malloc(size);
	cat1=strcat(cat1,path);
	setenv("tar",cat1,1);
	free(cat1);
}

int main (int argc, char *argv[]){

	if(argc!=2){

		exit(-1);
	}

	char ** path =decompose(argv[1],"/");

	char bufdir [PATH_MAX + 1];
	getcwd(bufdir,sizeof(bufdir));


	if(tar_file_exists(bufdir,path[0])){

			if(path[1]==NULL){	

				append_path(path[0]);
			}
		
	}

	
	printf("%s\n",getenv("tar"));


return 0;
}