#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "tar_nav.h"
#include "cd.h"
#include "print.h"

int run=1;
int d_stdout=1;
/*READS A LINE ENTERED IN THE TERMINAL AND RETURNS IT*/

char * get_line(){

	char * line = NULL;
	line= readline("");
	return line;
}

/*RETURN A STRING THAT CONTAINS ONLY THE LAST DIRECTORY FROM
GETCWD*/

char * get_last_dir(){

	char ** tokens;
	if(!current_dir_is_tar()){

		char bufdir [PATH_MAX + 1];
		getcwd(bufdir,sizeof(bufdir));
		tokens=decompose(bufdir,"/");
	}

	else{

		size_t size_tmp=strlen(getenv("tar"))+sizeof(char);
		char *tmp=malloc(size_tmp);
		strcpy(tmp,getenv("tar"));
		tokens=decompose(tmp,"/");

	}

	int cpt=0;

	while(tokens[cpt]!=NULL){

		cpt++;
	}
	char * ret=malloc(strlen(tokens[cpt-1])+sizeof(char));

	if(ret==NULL){

		perror("malloc");
		exit(-1);
	}

	ret=strcpy(ret,tokens[cpt-1]);
	free(tokens);
	return ret;

}

/*RETURNS THE FULL PATH OF THE CURRENT WORKING DIRECTORY*/

void pwd(){

	char bufdir [PATH_MAX + 1];
	getcwd(bufdir,sizeof(bufdir));
	char * entry=malloc(sizeof(bufdir)+sizeof(getenv("tar"))+sizeof("/"));
	memset(entry,0,sizeof(bufdir)+sizeof(getenv("tar")));
	entry= strcat(entry,bufdir);
	entry=strcat(entry,"/");
	entry = strcat(entry,getenv("tar"));
	prints(entry);
	prints("\n");
	free(entry);


}

/*HANDLES REDIRECTIONS : CHANGES THE DESCRIPTORS SO SATISFY THE REDIRACTIONS 
SPECIFIED IN PROMPT*/

char * redir(char * prompt){

	char * check_1 =malloc(strlen(prompt)+sizeof(char));
	strcpy(check_1,prompt);

	char ** tokens= decompose(check_1,">>");

	if(strcmp(tokens[0],prompt)!=0){	

		int fd=open(tokens[1],O_RDWR|O_APPEND|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

		if(fd<0){

			perror("open");
			exit(-1);
		}

		d_stdout=dup(STDOUT_FILENO);
		dup2(fd,STDOUT_FILENO);
		return tokens[0];
	}

	return "";
}

void reinit_descriptors(){

	dup2(d_stdout,STDOUT_FILENO);

}

/*PARSE TOKENS AND EXEC THE APPROPRIATE COMMAND*/

void parse (char ** tokens){

	/*WE HANDLE THE SPECIFIC CASES*/

	if(strcmp(tokens[0],"exit")==0){

		run =0;
		return;

	}

	if(strcmp(tokens[0],"pwd")==0  && current_dir_is_tar()){

		pwd();
		return;

	}

	/*CD CAN'T BE CALLED AFTER A FORK CAUSE THE CHANGES WOULDNT CARRY OVER TO
	THE FATHER PROCESS*/

	 if(strcmp(tokens[0],"cd")==0){

			cd(tokens[1]);
			return;			
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

				  		execvp(tokens[0],tokens);
					  	 		
				  	}

				  	exit(EXIT_FAILURE);

				  default: //FATHER

				  	//WAITING FOR THE SON TO TERMINATE	

				    wait(&w);
				  }
	}

}


int main (void){

	/*WE INITIALISE THE ENVIRONEMENT VAIRABLE THAT CONTAINS
	THE CURRENT TAR PATH*/

	setenv("tar","",1);
	prints("\n");

	while(run){

		/*ENTRY*/

		char * login = getlogin();
		char host [HOST_NAME_MAX];
		gethostname(host, HOST_NAME_MAX);

		prints("\033[1;32m[");
		prints(login);
		prints("@");
		prints(host);
		prints("\033[0m\e[1m ");
		char * last_dir=get_last_dir();
		prints(last_dir);
		prints("\033[0m\033[1;32m]$\033[0m");

		/*READING COMMAND*/

		char * prompt = get_line();

		/*HANDLING THE CASE WHERE NOTHING WAS WRITTEN*/

		if(strlen(prompt)==0){

			continue;
		}

		/*HANDLING REDIRECTIONS*/

		char * prompt_cpy =malloc(strlen(prompt)+sizeof(char));

		if(prompt_cpy==NULL){
			perror("malloc");
			exit(-1);
		}

		strcpy(prompt_cpy,prompt);
		char * prompt_clear=redir(prompt_cpy);
		free(prompt_cpy);
		char * prompt_check1;
		if(strcmp(prompt_clear,"")!=0){

			prompt_check1=malloc(strlen(prompt_clear)+sizeof(char));
			strcpy(prompt_check1,prompt_clear);

		}


		else{

			 prompt_check1=prompt;
		}



		/*DECOMPOSING THE COMMAND */

		char ** tokens = decompose(prompt_check1," ");

		/*PARSING THE COMMAND*/

		parse(tokens);

		/*FREE*/
		reinit_descriptors();
		free(prompt);
		free(last_dir);
		free(tokens);
		
	}

	return 0;
}