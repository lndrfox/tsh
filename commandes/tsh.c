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
char path_home [PATH_MAX + 1];;

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

/*RETURNS 1 IF AT LEAST AN ELEMENT OF TOKEN IS A STRING THAT CONTAINS .TAR
ELSE RETURNS*/

int args_contain_tar(char ** tokens){

	int cpt=0;
	while(tokens[cpt]!=NULL){

		if(string_contains_tar(tokens[cpt])){

			return 1;
		}
		
		cpt++;
	}

	return 0;
}

/*CUT THE SIZE FIRST CHAR FROM THE STRING PROMPT*/

void str_cut(char *prompt ,int size){

	int lenght= strlen(prompt);
	memmove(prompt,prompt+size,lenght -size +1);

}

/*HANDLES THE > , >> , 2> AND 2>> REDIRECTIONS FROM PROMPT
AND RETURNS A STRING THAT IS PROMPT WITHOUT THESE REDIRECTIONS*/

char * redir_out(char * prompt){

	d_stdout=dup(STDOUT_FILENO);//WE SAVE STDOUT BEFORE USING DUP2 TO RESTORE IT LATER
	d_stderr=dup(STDERR_FILENO); //WE SAVE STDERR BEFORE USING DUP2 TO RESTORE IT LATER

	/*WE COPY THE PROMPT TO NOT BREAK IT*/

	char * prompt_cpy=malloc(strlen(prompt)+sizeof(char));

	/*ERROR MANGEMENT*/

	if(prompt_cpy==NULL){

		perror("malloc");
		exit(-1);
	}

	/*WE SAVE THE ADRESS OF THE FIRST CHAR OF PROMPT_CPY IN
	SAVE_POS. WE DECLARE ANCHOR AS NULL
	FLAG WILL BE RAISED IF WE ARE IN THE CASE OF A >> OR 2>> REDIRECTION*/

	strcpy(prompt_cpy,prompt);
	char * save_pos =prompt_cpy;
	char * anchor =NULL;
	int flag=0;

	/*----- WHILE WE CAN FIND A ">" IN PROMPT_CPY, WE REPEAT THE LOOP-----*/

	do{

		/*WE LOOK FOR THE FIRST OCC OF ">" IN PROMPT_CPY AND STORE IT IN ANCHOR*/

		anchor = strchr(prompt_cpy,'>');

		/*ERROR MANGEmENT*/

		if(anchor ==NULL){

			return(save_pos);
		}

		/*WE RAISE THE FLAG ONLY IF THE NEXT CHARACTER IS ALSO ">" WHICH 
		MEANS WE ARE IN A ">>" SITUATION*/

		if(anchor[1]=='>'){
			flag=1;
		}

		/*PATH IS WERE WE BUILD THE STRING OF WERE WE WILL REDIRECT*/
		
		char * path=malloc(sizeof(char));

		/*ERROR MANGEMENT*/

		if(path==NULL){
			perror("malloc");
			exit(-1);
		}

		memset(path,0,sizeof(char));

		//CPT FOR ANCHOR

		int cpt;

		/*IF THE FLAG WAS RAISED, WE NEED TO IGNORE THE SECOND ">" CHARACTER
		SO WE START CPT AT 2 INSTEAD OF 1*/

		if(flag){
			cpt=2;
		}
		else{
			cpt=1;
		}
				
		int cpt_path=0;//CPT FOR PATH
		int len_cut=cpt;//THIS IS THE LENGHT OF THE SUB STRING WE WANT TO CUT FROM THE PROMPT
						// INCLUDING THE > SYMBOL AND THE PATH

		/*------ WE LOOP WHILE THE NEXT CHARACTER DOESN'T
		INDICATE ANOTHER REDIRACTION OR THE END OF THE STRING------*/

		while(anchor[cpt]!='\0' && anchor [cpt] != '>' 
								&& anchor [cpt] != '<'){

			/*IF THE STRING BEGINS BY A " " WE IGNORE IT, THE VALUE
			OF CPT WE CHECK DEPENDS IF THE FLAG WAS RAISED OR NOT*/

			if(flag){

				if(cpt == 2 && anchor[cpt]==' '){
					cpt++;
					len_cut++;
					continue;
				}
			}	

			else{
				if(cpt == 1 && anchor[cpt]==' '){
					cpt++;
					len_cut ++;
					continue;
					}
				}

			/*IF WE ENCOUNTER A " " NOT AT THE BEGGINIGN THEN
			WE ARE DONE BULIDING THE STRING*/

			if(anchor[cpt]==' ' && cpt!=1 && cpt !=2){
				len_cut++;
				break;
			}

			/*WE FILL THE STRING, HANDLES CPT AND REALLOC ENOUGH
			MEMORY TO FILL THE STRONG AT THE NEXT ITERATION OF THE WHILE*/

			path[cpt_path]=anchor[cpt];
			cpt_path++;
			path=realloc(path,(cpt_path+1)*sizeof(char));
			cpt++;

			}

		/*WE END THE STRING*/

		path[cpt_path]='\0';
		len_cut+=strlen(path);

		int flag_err=0; //THIS FLAG IS RAISED IF WE NEED TO REDIRECT STDERR_FILENO

		/*------IF FLAG IS RAISED THEN WE HAVE EITHER >> OR 2>> SO THE
		DESCRIPTOR MUST BE OPENED IN APPEND MODE ------*/

		if(flag){

			/*WE OPEN THE DESCRIPTOR*/

			int fd=open(path,O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

			/*IF WE NEED TO REDIRECT STDERR_FILENO*/

			if(anchor[-1]=='2'){
				dup2(fd,STDERR_FILENO);
				flag_err=1;//WE RAISE THE ERR FLAG
			}

			/*IF WE NEED TO REDIRECT STDOUT_FILENO*/

			else{
				dup2(fd,STDOUT_FILENO);
			}
		}

		/*------IF FLAG IS not RAISED THEN WE HAVE EITHER > OR 2> SO THE
		DESCRIPTOR MUST NOT BE OPENED IN APPEND MODE ------*/

		else{
			int fd=open(path,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

			/*IF WE NEED TO REDIRECT STDERR_FILENO*/

			if(anchor[-1]=='2'){
				dup2(fd,STDERR_FILENO);
				flag_err=1;//WE RAISE THE ERR FLAG
			}

			/*IF WE NEED TO REDIRECT STDOUT_FILENO*/

			else{
				
				dup2(fd,STDOUT_FILENO);
			}
		}

		/*IF THE ERR FLAG WAS RAISED, WE ALSO NEED TO CUT THE 2*/	

		if(flag_err){
			len_cut++;
			str_cut(&(anchor[-1]),len_cut);
		}

		/*ELSE WE CUT OUT THE > SYMBOLE + THE PATH*/
		else{
			str_cut(anchor,len_cut);
		}

		/*WE LOWER THE FLAG*/

		flag=0;
		free(path);

	}while(anchor!=NULL);

	return save_pos;

}

char * redir_in(char * prompt){

	d_stdin=dup(STDIN_FILENO);//WE SAVE STDOUT BEFORE USING DUP2 TO RESTORE IT LATER

	/*WE COPY THE PROMPT TO NOT BREAK IT*/

	char * prompt_cpy=malloc(strlen(prompt)+sizeof(char));

	/*ERROR MANGEMENT*/

	if(prompt_cpy==NULL){

		perror("malloc");
		exit(-1);
	}

	/*WE SAVE THE ADRESS OF THE FIRST CHAR OF PROMPT_CPY IN
	SAVE_POS. WE DECLARE ANCHOR AS NULL*/

	strcpy(prompt_cpy,prompt);
	char * save_pos =prompt_cpy;
	char * anchor =NULL;

	/*----- WHILE WE CAN FIND A "<" IN PROMPT_CPY, WE REPEAT THE LOOP-----*/

	do{
		/*WE LOOK FOR THE FIRST OCC OF "<" IN PROMPT_CPY AND STORE IT IN ANCHOR*/

		anchor = strchr(prompt_cpy,'<');

		/*ERROR MANGEMENT*/

		if(anchor ==NULL){
			return(save_pos);	
		}

		/*PATH IS WERE WE BUILD THE STRING OF WERE WE WILL REDIRECT*/
		
		char * path=malloc(sizeof(char));

		/*ERROR MANGEMENT*/

		if(path==NULL){
			perror("malloc");
			exit(-1);
		}

		memset(path,0,sizeof(char));

		//CPT FOR ANCHOR

		int cpt=1;
				
		int cpt_path=0;//CPT FOR PATH
		int len_cut=cpt;//THIS IS THE LENGHT OF THE SUB STRING WE WANT TO CUT FROM THE PROMPT
						// INCLUDING THE < SYMBOL AND THE PATH

		/*------ WE LOOP WHILE THE NEXT CHARACTER DOESN'T
		INDICATE ANOTHER REDIRACTION OR THE END OF THE STRING------*/

		while(anchor[cpt]!='\0' && anchor [cpt] != '<'){

			/*IF THE STRING BEGINS BY A " " WE IGNORE IT*/

			if(cpt == 1 && anchor[cpt]==' '){
				cpt++;
				len_cut ++;
				continue;
			}

			/*IF WE ENCOUNTER A " " NOT AT THE BEGGINIGN THEN
			WE ARE DONE BULIDING THE STRING*/

			if(anchor[cpt]==' ' && cpt!=1 ){
				len_cut++;
				break;
			}

			/*WE FILL THE STRING, HANDLES CPT AND REALLOC ENOUGH
			MEMORY TO FILL THE STRONG AT THE NEXT ITERATION OF THE WHILE*/

			path[cpt_path]=anchor[cpt];
			cpt_path++;
			path=realloc(path,(cpt_path+1)*sizeof(char));
			cpt++;

			}

		/*WE END THE STRING*/

		path[cpt_path]='\0';
		len_cut+=strlen(path);

		int fd=open(path,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
		dup2(fd,STDIN_FILENO);

		/* WE CUT OUT THE < SYMBOLE + THE PATH*/
		
		str_cut(anchor,len_cut);
		free(path);

	}while(anchor!=NULL);

	return save_pos;
}

/*REINITIALIZE THE DESCRIPTORS THAT MIGHT HAVE BEEN CHANGED
BECAUS EOF REDIRECTIONS*/

void reinit_descriptors(){

	dup2(d_stdout,STDOUT_FILENO);
	dup2(d_stdin,STDIN_FILENO);
	dup2(d_stderr,STDERR_FILENO);

}

/*HANDLES REDIRECTIONS : CHANGES THE DESCRIPTORS SO SATISFY THE REDIRECTIONS 
SPECIFIED IN PROMPT*/

char * redir(char * prompt){
	
	char * out =redir_out(prompt);
	char *ret =redir_in(out);
	free(out);
	return ret;
	
}

/*GIVEN TOKENS A CHAR ** CONTAINING A COMMAND NAMES AND IT'S ARGUMENT AND FINISH BY NULL,
EXEC_TAR_OR_BIN DECIDES IF THE ORIGINAL VERSION OR THE TAR VERSION OF THE COMMAND
SHOULD BE CALLED AND CALLS IT WITH EXEC*/

void exec_tar_or_bin(char ** tokens){

	//IF WE ARE WORKING WITH TAR FILES

	char * true_path=malloc(strlen(path_home)+sizeof(char));
	true_path=strcpy(true_path,path_home);
	true_path=realloc(true_path,strlen(true_path)+2*(sizeof(char)));
	true_path=strcat(true_path,"/");
	true_path=realloc(true_path,strlen(true_path)+strlen(tokens[0])+sizeof(char));
	true_path=strcat(true_path,tokens[0]);

	if((current_dir_is_tar() || args_contain_tar(tokens)) && access(true_path,F_OK|X_OK)==0){

		//WE CHECK IF THE TAR COMMAND EXISTS/IS HANDLED

		execv(true_path,tokens);

					  		
	}

	//ELSE IF WE ARE NOT WORKING WITH A TAR FILE

	else{

		execvp(tokens[0],tokens);
						  	 		
	}
}

/*GIVEN A CHAR ** TOKENS CONTAINING A COMMAND NAMES, ITS ARGUMENTS AND FINISHING BY NULL
EXEC FORKS AND THE CHILD PROCESS EXECUTES THE COMMAND IN TOKENS*/

void exec(char ** tokens){

	/*WE FORK TO GET A NEW PROCESS*/

	int r,w;
	r=fork();

	switch(r){

		case -1: //ERROR

			perror("fork");
			exit(EXIT_FAILURE);

		case 0: //SON

			exec_tar_or_bin(tokens);
			exit(EXIT_FAILURE);

		default: //FATHER

			//WAITING FOR THE SON TO TERMINATE	
			wait(&w);
	}

}

/*EXECUTES THE COMMANDS SPECIFIED BY TOKEN_1 AND TOKEN_2 WITH A PIPE
IF NEXT IS NULL THEN IT STOPS THERE. IF THERE IS ANOTHER COMMAND
TO COMBINE IN NEXT, IT CALLS ITSELF RECURSIVELY*/


void exec_pipe( char ** token_1, char ** token_2, char ** next){

	/*WE CREATE THE PIPE*/

	int fd[2];

	/*ERROR MANGEMENT*/

	if(pipe(fd)<0){

		perror("pipe");
		exit(-1);
	}

	/*WE FORK A FIRST TIME*/

	int r,r2,w;
	r=fork();

	switch(r){

		case -1: //ERROR

			perror("fork");
			exit(EXIT_FAILURE);

		case 0: //SON 

			/*WE NEED TO FORK A SECOND TIME TO BE ABLE TO HAVE
			TWO NEW PROCESS*/
			
			r2=fork();

			switch(r2){

				case -1: //ERROR

					perror("fork");
					exit(EXIT_FAILURE);

				case 0: //SON, WRITER

					close(fd[0]);
					dup2(fd[1],STDOUT_FILENO);
					exec_tar_or_bin(token_1);
					exit(EXIT_FAILURE);

				default: //FATHER, READER

					close(fd[1]);
					dup2(fd[0],STDIN_FILENO);

					/*IF THERE IS NOTHING ELSE IN NEXT, THEN WE SIMPLY 
					EXEC THE COMMAND*/

					if(next[1]==NULL){

						exec_tar_or_bin(token_2);
					}

					/*ELSE WE DECOMPOSE THE COMMAND IN NEXT[1] AND
					THEN WE CALL EXEC_PIPES RECURSIVELY WITH TOKEN_2, TOKEN_NEXT
					AND NEXT[1] FOR THE ARGUMENT NEXT*/

					else{

						char * token_cpy= malloc(strlen(next[1])+sizeof(char));
						strcpy(token_cpy,next[1]);

						char ** tokens_next=decompose(token_cpy," ");

						exec_pipe(token_2,tokens_next,&(next[1]));
					}
					
					exit(EXIT_FAILURE);
			}
			
		default: //FATHER

			close(fd[0]);
			close(fd[1]);
			wait(&w);			
	}

}

/*PARSE THE PROMPT AND CALLS THE APPROPRIATE FUNCTIONS
TO TAKE CARE OF PIPES AND CHOOSE WHAT COMMAND SHOULD BE CALLED WITH EXEC*/

void parse (char * prompt){

	/*WE COPY THE PROMPT BECAUSE DECOMPOSE WILL BREAK THE STRING*/

	char * prompt_cpy=malloc(strlen(prompt)+sizeof(char));
	strcpy(prompt_cpy,prompt);

	/*ERROR MANAGMENT*/

	if(prompt_cpy==NULL){
		perror("malloc");
		exit(-1);
	}

	/*WE DECOMPOSE THE PORMPT ACCORDING TO  | SO WE CAN CHECK IF 
	PIPES SHOULD BE USED*/

	char ** tokens_pipe=decompose(prompt_cpy,"|");

	/*WE COPY THE TOKEN BECAUSE WE DON't WANT TO BREAK IT
	WHEN WE USE DECOMPOSE*/

	char * token_1 =malloc(strlen(tokens_pipe[0])+sizeof(char));
	strcpy(token_1,tokens_pipe[0]);

	/*WE DECOMPOSE THE FIRST TOKEN SO WE CAN LOOK FOR 
	CD AND EXIT*/

	char ** tokens=decompose(token_1," ");

	/*ERROR MANGEMENT*/

	if(tokens==NULL){

		perror("malloc");
		exit(-1);
	}

	/*IF THE COMMAND IS EXIT*/

	if(strcmp(tokens[0],"exit")==0){

		run =0;
		free(tokens_pipe);
		free(tokens);
		free(token_1);
		free(prompt_cpy);
		return;
	}

	/*IF THE COMMAND IS PWD*/

	if(strcmp(tokens[0],"pwd")==0  && current_dir_is_tar()){

		pwd();
		free(tokens_pipe);
		free(tokens);
		free(token_1);
		free(prompt_cpy);
		return;
	}

	/*CD CAN'T BE CALLED AFTER A FORK CAUSE THE CHANGES WOULDNT CARRY OVER TO
	THE FATHER PROCESS*/

	 if(strcmp(tokens[0],"cd")==0){

			cd(tokens[1]);
			free(tokens_pipe);
			free(tokens);
			free(token_1);
			free(prompt_cpy);
			return;			
		}

	/*ELSE WE TAKE CARE OF PIPES AND THEN EXECUTE THE COMMANDS*/

	else{

		/*IF WE HAVE PIPES*/

		if(tokens_pipe[1]!=NULL){

			/*WE COPY THE NEXT PART TO NOT BREAK IT EITHER*/

			char * token_2=malloc(strlen(tokens_pipe[1])+sizeof(char));
			strcpy(token_2,tokens_pipe[1]);

			/*ERROR MANGEMENT*/

			if(token_2==NULL){

				perror("malloc");
				exit(-1);
			}

			/*WE DECOMPOSE TOKEN_2 WITH " "*/

			char ** tokens_2 = decompose(token_2," ");

			/*WE CALL EXEC_PIPE WITH OUR TWO TOKENS AND TOKENS_PIPE[1] FOR THE ARGUMENT
			NEXT*/

			exec_pipe(tokens,tokens_2,&(tokens_pipe[1]));

			free(tokens_2);
			free(token_2);
		}

		/*IF WE DON'T HAVE PIPES*/

		else{

			exec(tokens);

		}
	}

	/*FREES*/

	free(tokens_pipe);
	free(tokens);
	free(token_1);
	free(prompt_cpy);

}


int main (void){

	/*WE INITIALISE THE ENVIRONEMENT VAIRABLE THAT CONTAINS
	THE CURRENT TAR PATH*/

	setenv("tar","",1);
	prints("\n");
	getcwd(path_home,sizeof(path_home));

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

			prints("\n");
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

		/*------PARSING THE COMMAND------*/

		parse(prompt_clear);
		reinit_descriptors();

		/*------FREE------*/

		free(prompt);
		free(last_dir);
		free(prompt_clear);
		free(prompt_cpy);
		
	}

	return 0;
}