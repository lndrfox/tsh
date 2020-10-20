#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
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


}


int main (void){

	char bufdir [PATH_MAX + 1];
	setenv("tar","",1);

	while(run){

		getcwd(bufdir,sizeof(bufdir));
		printf("\033[1;32m\n%s >\033[0m\n",bufdir);

		char * prompt = get_line();
		char ** tokens = decompose(prompt," ");

		parse(tokens);
		
		
	}

	return 0;
}