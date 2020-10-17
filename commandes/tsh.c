#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "tar.h"



char * get_line();

char ** decompose(char * prompt, char * delimiter);


/*READS A LINE ENTERED IN THE TERMINAL AND RETURNS IT*/

char * get_line(){

	char * line = NULL;

	if(line!=NULL){

		free(line);
	}

	line= readline("");
	return line;
}

/*DECOMPOSE THE STRING PROMPT ACCORDING TO THE STRING DELIMITER AND RETURNS THE TOKENS
IN CHAR ** TOKEN ENDING BY NULL*/

char ** decompose(char * prompt, char * delimiter){

	char ** tokens= calloc (1,sizeof(char *));

	if(tokens ==NULL){

		exit (-1);
	}

	char * token = strtok(prompt, delimiter);

	tokens[0]=token;

	int cpt_tokens=1;

	while((token =strtok(NULL, delimiter) ) !=NULL){

		tokens[cpt_tokens]= token;
		tokens =realloc(tokens, sizeof(tokens)+sizeof(char* ));
		cpt_tokens++;

	}

	tokens[cpt_tokens]=NULL;
	return tokens;


}

int main (int argc, char *argv[]){

	char bufdir [PATH_MAX + 1];

	while(1){

		getcwd(bufdir,sizeof(bufdir));
		printf("\033[1;32m\n%s >\033[0m\n",bufdir);
		char * prompt = get_line();
		char ** tokens = decompose(prompt," ");
		printf("\n%s",tokens[0]);



		
	}

	return 0;
}