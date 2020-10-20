#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include "tar.h"
#include "tar_nav.h"


int run=1;


/*READS A LINE ENTERED IN THE TERMINAL AND RETURNS IT*/

char * get_line(){

	char * line = NULL;
	line= readline("");
	return line;
}




/*PARSE TOKENS AND EXEC THE APPROPRIATE COMMAND*/

void parse (char ** tokens){

	if(strcmp(tokens[0],"exit")==0){

		run =0;

	}

	if(strcmp(tokens[0],"mkdir")==0){

		if(current_dir_is_tar()){

			int r,w;
			r=fork();

			switch(r){
				  case -1: 

				  	perror("fork");
				    exit(EXIT_FAILURE);

				  case 0: //fils

					execv("mkdir",tokens);
				    exit(EXIT_SUCCESS);

				  default: //pere

				    wait(&w);
				  }


			
		}

	}


}


int main (void){

	char bufdir [PATH_MAX + 1];
	setenv("tar","c.tar",1);

	while(run){

		getcwd(bufdir,sizeof(bufdir));
		printf("\033[1;32m\n%s >\033[0m\n",bufdir);

		char * prompt = get_line();

		if(strlen(prompt)==0){

			continue;
		}
		char ** tokens = decompose(prompt," ");

		parse(tokens);
		
		
	}

	return 0;
}