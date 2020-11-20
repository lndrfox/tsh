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
int d_stdin=0;
int d_stderr=2;

/*READS A LINE ENTERED IN THE TERMINAL AND RETURNS IT*/

char * get_line(){

	char * line = NULL;
	line= readline("");
	return line;
}

/*RETURN A STRING THAT CONTAINS ONLY THE LAST DIRECTORY FROM
GETCWD*/

char * get_last_dir(){

	/*IF THE CURRENT DIRECTORY ISN'T A TAR, WE DECOMPOSE BUFDIR IN TOKENS*/

	char ** tokens;
	size_t size_tmp=strlen(getenv("tar"))+sizeof(char);
	char *tmp=malloc(size_tmp);

		if(tmp==NULL){

			perror("malloc");
			exit(-1);
		}

	if(!current_dir_is_tar()){

		char bufdir [PATH_MAX + 1];
		getcwd(bufdir,sizeof(bufdir));
		tokens=decompose(bufdir,"/");
	}

	/*ELSE WE DECOMPSE THE CURRENT PATH IN THE TAR */

	else{

		memset(tmp,0,size_tmp);
		strcpy(tmp,getenv("tar"));
		tokens=decompose(tmp,"/");
	}

	/*WE GET TO THE LAST TOKEN OF TOKENS TO GET THE LAST DIRECTORY OF THE PATH*/

	int cpt=0;

	while(tokens[cpt]!=NULL){

		cpt++;
	}
	/*WE COPY THE LAST DIRECTORY IN A STRING AND WE RETURN IT*/

	char * ret=malloc(strlen(tokens[cpt-1])+sizeof(char));

	if(ret==NULL){

		perror("malloc");
		exit(-1);
	}

	ret=strcpy(ret,tokens[cpt-1]);
	free(tokens);
	free(tmp);
	return ret;

}

/*RETURNS THE FULL PATH OF THE CURRENT WORKING DIRECTORY*/

void pwd(){

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


}

/*HANLDES > AND 2>> REDIRECTIONS, RETURNS THE COMMAND WITHOUT THE 
REDIRECTION PART OR "" IF NO REDIRECTION HAPPENED*/


char * redir_out(char *prompt){
	/*WE SAVE THE FORMER DESCRIPTOR*/
	d_stdout=dup(STDOUT_FILENO);
	d_stderr=dup(STDERR_FILENO);

	/*WE COPY THE STRING BECAUSE STRTOK IN DECOMPSOE MIGHT MESS WITH IT*/

	char * check =malloc(strlen(prompt)+sizeof(char));

	if(check==NULL){
		perror("malloc");
		exit(-1);
	}

	strcpy(check,prompt);

	/*WE DECOMPOSE LOOKING FOR ERROR REDIRECTIONS*/

	char ** tokens= decompose(check,">");

	/*IF A REDIRECTION WAS FOUND*/

	if(strcmp(tokens[0],prompt)!=0){

		int cpt=1;
		int flag=0;
		while(tokens[cpt]!=NULL){

			int fd=-1;

			/*IF THE NEXT REDIRECTION IS AN ERRROR REDIRECTION
			WE IGNORE THE 2 AT THE END OF THE TOKEN*/

			if(	strlen(tokens[cpt])>=2 &&
				tokens[cpt+1]!=NULL &&
				tokens[cpt][strlen(tokens[cpt])-1]=='2'&&
				tokens[cpt][strlen(tokens[cpt])-2]==' '){

				char * path=malloc(strlen(tokens[cpt])-2+sizeof(char));

				if(path==NULL){

					perror("malloc");
					exit(-1);
				}

				memset(path,0,strlen(tokens[cpt])-2+sizeof(char));
				strncpy(path,tokens[cpt],strlen(tokens[cpt])-2);

				/*WE OPEN THE REDIRECTION FILE AND CREATE IT IF NEEDED*/
				fd=open(path,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

				/*ERROR MANGEMENT*/
				if(fd<0){
					perror("open");
					exit(-1);
				}
			}

			/*IF NOT WE SIMPLY USE THE CURRENT TOKEN TO OPEN THE DESCRIPTOR*/

			else{
				/*WE OPEN THE REDIRECTION FILE AND CREATE IT IF NEEDED*/
				fd=open(tokens[cpt],O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

				/*ERROR MANGEMENT*/
				if(fd<0){
					perror("open");
					exit(-1);
				}
			}

			/*IF THE LAST CHAR OF THE FORMER TOKEN WAS 2 THEN WE
			NEED TO TO A REDIR OF STDERR*/

			if(	strlen(tokens[cpt-1])>=2 &&
				tokens[cpt-1][strlen(tokens[cpt-1])-1]=='2'&&
				tokens[cpt-1][strlen(tokens[cpt-1])-2]==' '){
				if(cpt==1){
					flag=1;
				}
				/*WE CHANGE THE DESCRITPOR*/
				dup2(fd,STDERR_FILENO);

			}

			/*ELSE THE REDIR IS ON STDOUT*/

			else{
				/*WE CHANGE THE DESCRITPOR*/
				dup2(fd,STDOUT_FILENO);
			}

			cpt++;

		}

		/*----WE RETURN THE PROMPT WITHOUT THE REDIRECTION PART-----*/

		/*IF THE FLAG WAS RAISED, WE NEED TO REMOVE THE 2 FROM TOKENS[0]*/
		if(flag){

			char * ret= malloc(strlen(tokens[0])-2+sizeof(char));

			if(ret==NULL){
				perror("malloc");
				exit(-1);
			}

			strncpy(ret,tokens[0],strlen(tokens[0])-2);
			free(check);
			free(tokens);
			return ret;
		}

		/*ELSE WE RETURN TOKENS[0] BUT WE NEED TO MAKE A COPY TO BE ABLE TO FREE EVERYTHING
		THAT NEED TO BE FREED*/

		char * ret_1=malloc(strlen(tokens[0])+sizeof(char));

		if(ret_1==NULL){

			perror("malloc");
			exit(-1);
		}

		strcpy(ret_1,tokens[0]);
		free(check);
		free(tokens);
		return ret_1;

	}

	/*WE RETURN "" IF NO REDIR WAS FOUND AND WE NEED TO TO
	A MALLOC TO BE ABLE TO FREE IT LATER IN THE MAIN LOOP*/

	free(check);
	free(tokens);
	char * ret_v=malloc(2);

	if(ret_v==NULL){

		perror("malloc");
		exit(-1);
	}

	strcpy(ret_v,"");
	return ret_v;
	return "";
}

char * redir_in(char * prompt){

	/*WE SAVE THE FORMER DESCRIPTOR*/
	d_stdin=dup(STDIN_FILENO);
	/*WE COPY THE STRING BECAUSE STRTOK IN DECOMPSOE MIGHT MESS WITH IT*/
	char * check =malloc(strlen(prompt)+sizeof(char));
	if(check==NULL){
		perror("malloc");
		exit(-1);
	}

	strcpy(check,prompt);

	/*WE DECOMPOSE LOOKING FOR ERROR REDIRECTIONS*/

	char ** tokens= decompose(check,"<");
	/*IF A REDIRECTION WAS FOUND*/
	if(strcmp(tokens[0],prompt)!=0){

		int cpt=1;

		while(tokens[cpt]!=NULL){

			/*WE OPEN THE REDIRECTION FILE AND CREATE IT IF NEEDED*/
			int fd=open(tokens[cpt],O_RDWR);

			/*ERROR MANGEMENT*/
			if(fd<0){
				perror("open");
				free(check);
				free(tokens);
				char * ret_e=malloc(2);
				strcpy(ret_e,"");
				return ret_e;
			}

			dup2(fd,STDIN_FILENO);
			cpt++;
		}
		/*WE RETURN THE PROMPT WITHOUT THE REDIRECTION PART*/

		char * ret = malloc(strlen(tokens[0])+sizeof(char));
		if(ret==NULL){

			perror("malloc");
			exit(-1);
		}
		strcpy(ret,tokens[0]);
		free(check);
		free(tokens);
		return ret;
	}

	free(check);
	free(tokens);
	char * ret_v=malloc(2);
	strcpy(ret_v,"");
	return ret_v;
}


void reinit_descriptors(){

	dup2(d_stdout,STDOUT_FILENO);
	dup2(d_stdin,STDIN_FILENO);
	dup2(d_stderr,STDERR_FILENO);

}

/*HANDLES REDIRECTIONS : CHANGES THE DESCRIPTORS SO SATISFY THE REDIRACTIONS 
SPECIFIED IN PROMPT*/

char * redir(char * prompt){

	/*WE COPY THE PROMPT TO NOT BREAK IT*/

	char * cpy=malloc(strlen(prompt)+sizeof(char));
	strcpy(cpy,prompt);

	/*WE START BY LOOKING FOR > REDIRECTIONS*/

	char * ret_out = redir_out(cpy);
	char * ret_in;
	free(cpy);

	/*IF THERE ARE NONE, WE LOOK FOR < REDIRECTIONS IN THE ORIGINAL PROMPT AND 
	RETURN THE STRING OBTAINED*/

	if(strcmp(ret_out,"")==0){

		ret_in=redir_in(prompt);
		free(ret_out);
		return ret_in;
	}

	/*IF THERE WERE > REDIRACTIONS*/

	else{

		/*WE LOOK FOR < REDIR LOOKING IN THE PROMPT
		OBTAINED FROM THE FORMER SEARCH*/

		ret_in=redir_in(ret_out);

		/*IF WE FIND NONE WE RETURN THE RESULT FROM THE FORMER SEARCH*/

		if(strcmp(ret_in,"")==0){
			free(ret_in);
			return ret_out;
		}

		/*OTHERWISE WE RETURN THE STRING OBTAINED*/
		free(ret_out);
		return ret_in;
	}
	
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

		/*-----HANDLING REDIRECTIONS------*/

		/*WE COPY THE PROMPT TO NOT BREAK IT*/
		char * prompt_cpy =malloc(strlen(prompt)+sizeof(char));

		/*ERROR MANGEMENT*/
		if(prompt_cpy==NULL){
			perror("malloc");
			exit(-1);
		}

		strcpy(prompt_cpy,prompt);

		/*WE CALL THE REDIR FUNCTION*/

		char * prompt_clear=redir(prompt_cpy);
		char ** tokens ;

		/*IF A REDIRECTION HAPPENED, WE USE THE RETURNED STRING AS OUR PROMPT FOR
		THE REST OF THE PROGRAM*/

		if(strcmp(prompt_clear,"")!=0){

			tokens = decompose(prompt_clear," ");
		}

		/*ELSE WE SIMPLY USE OUR PROMPT*/
		
		else{

			tokens = decompose(prompt," ");
		}

		/*PARSING THE COMMAND*/

		parse(tokens);

		/*FREE*/
		reinit_descriptors();
		free(prompt);
		free(last_dir);
		free(prompt_cpy);
		free(prompt_clear);
		free(tokens);
		
	}

	return 0;
}