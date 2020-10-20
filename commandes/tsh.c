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

int run=1;


/*READS A LINE ENTERED IN THE TERMINAL AND RETURNS IT*/

char * get_line(){

	char * line = NULL;
	line= readline("");
	return line;
}

/*DECOMPOSE THE STRING PROMPT ACCORDING TO THE STRING DELIMITER AND RETURNS THE TOKENS
IN CHAR ** TOKEN ENDING BY NULL*/

char ** decompose(char * prompt, char * delimiter){

	/* THIS WERE WE STORE ALL THE TOKENS*/

	char ** tokens= calloc (1,sizeof(char *));

	/*ERROR MANAGEMENT*/

	if(tokens ==NULL){

		exit (-1);
	}

	/*WE CALL STRTOK FOR THE FIRST TIME*/

	char * token = strtok(prompt, delimiter);

	/*WE NEED TO STORE THE FIRST RESULT OF STTOK TO NOT LOSE ANYTHING*/

	tokens[0]=token;

	int cpt_tokens=1;

	/*AS LONG AS THERE IS SOMETHING TO DECOMPOSE WE TOK AND STORE IT IN TOKENS*/

	while((token =strtok(NULL, delimiter) ) !=NULL){

		tokens[cpt_tokens]= token;
		tokens =realloc(tokens, sizeof(tokens)+sizeof(char* ));
		cpt_tokens++;

	}

	/*WE ADD NULL AT THE END OF TOKENS */

	tokens[cpt_tokens]=NULL;

	return tokens;


}



/*PARSE TOKENS AND EXEC THE APPROPRIATE COMMAND*/

void parse (char ** tokens){

	if(strcmp(tokens[0],"exit")==0){

		run =0;

	}


}

/*CHECKS IF THE STRING STRING IS A TAR PATH */

int string_contains_tar(char * string){

	return strstr(string,".tar")!=NULL;
}

/*CHECK IF THE CURRENT WORKING DIRECTORY IS INSIDE OF A TAR*/

int current_dir_is_tar(){


	char bufdir [PATH_MAX + 1];
	getcwd(bufdir,sizeof(bufdir));
	char ** dir =decompose(bufdir, "/");

	int cpt=0;

	while(dir[i]!=NULL){

		if(string_contains_tar(dir[i])){

			return 1;
		}

	}

	return 0;
}

int main (int argc, char *argv[]){

	char bufdir [PATH_MAX + 1];

	while(run){

		getcwd(bufdir,sizeof(bufdir));
		printf("\033[1;32m\n%s >\033[0m\n",bufdir);

		char * prompt = get_line();
		char ** tokens = decompose(prompt," ");

		parse(tokens);

		
	}

	return 0;
}