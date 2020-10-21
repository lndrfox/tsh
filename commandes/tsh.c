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
#include "cd.h"


int run=1;


/*READS A LINE ENTERED IN THE TERMINAL AND RETURNS IT*/

char * get_line(){

	char * line = NULL;
	line= readline("");
	return line;
}




/*PARSE TOKENS AND EXEC THE APPROPRIATE COMMAND*/

void parse (char ** tokens){

	/*WE HANDLE THE SPECIFIC CASE OF EXIT*/

	if(strcmp(tokens[0],"exit")==0){

		run =0;

	}

	 if(strcmp(tokens[0],"cd")==0){


		if(tokens[1]!=NULL ){

			if(string_contains_tar(tokens[1])||  current_dir_is_tar()){

				cd(tokens[1]);
			}
		}


	}

	else{

			/*WE FORK TO GET A NEW PROCESS*/

			int r,w;
			r=fork();

			switch(r){


				  case -1: //ERROR

				  	perror("fork");
				    exit(EXIT_FAILURE);

				  case 0: //SON

				  //IF WE ARE WORKING WITH TAR FILES

				  	if(current_dir_is_tar()){

				  		//WE CHECK IF THE TAR COMMAND EXISTS/IS HANDLED

				  		if(access(tokens[0],F_OK|X_OK)==0){

				  			//EXEC THE COMMAND

				  			execv(tokens[0],tokens);

				  		}
				  		
				  	}


				  	//ELSE IF WE ARE NOT WORKING WITH A TAR FILE

				  	else{

				  		//BULDING /BIN/COMMAND STRING

				  		char * cat = malloc(strlen("/bin/")+strlen(tokens[0]));

				  		memset(cat,0, strlen(cat));
				  		cat=strcat(cat,"/bin/");
				  		cat=strcat(cat,tokens[0]);

				  		//CHECKING IF THE COMMAND EXISTS IN /BIN/

				  		if(access(tokens[0],F_OK|X_OK)==0){

				  			//EXEC THE COMMAND

				  			execv(cat,tokens);
				  		}
				  		
				  	}

				  	exit(EXIT_FAILURE);


				  default: //FATHER

				  	//WAITING FOR THE SON TO TERMINATE	

				    wait(&w);
				  }

			



	}


}


int main (void){

	char bufdir [PATH_MAX + 1];
	setenv("tar","",1);

	while(run){

		/*PROMPT*/

		getcwd(bufdir,sizeof(bufdir));

		char * entry=malloc(sizeof(bufdir)+sizeof(getenv("tar")));
		memset(entry,0,strlen(entry));
		entry= strcat(entry,bufdir);
		entry = strcat(entry,getenv("tar"));

		printf("\033[1;32m\n%s >\033[0m\n",entry);

		/*READING COMMAND*/

		char * prompt = get_line();

		/*HANDLING THE CASE WHERE NOTHING WAS WRITTEN*/

		if(strlen(prompt)==0){

			continue;
		}

		/*DECOMPOSING THE COMMAND */
		char ** tokens = decompose(prompt," ");

		/*PARSING THE COMMAND*/

		parse(tokens);
		
		
	}

	return 0;
}