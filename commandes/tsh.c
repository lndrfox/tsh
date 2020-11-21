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

void str_cut(char *prompt ,int size){

	int lenght= strlen(prompt);
	memmove(prompt,prompt+size,lenght -size +1);


}


char * redir_out(char * prompt){

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
	int flag=0;

	/*----- WHILE WE CAN FIND A ">" IN PROMPT_CPY, WE REPEAT THE LOOP-----*/

	do{

		/*WE LOOK FOR THE FIRST OCC OF ">" IN PROMPT_CPY AND STORE IT IN ANCHOR*/

		anchor = strchr(prompt_cpy,'>');

		/*ERROR MANGEmENT*/

		if(anchor ==NULL){
				
			break;
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
		int len_cut=cpt;

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
		int flag_err=0;

		if(flag){
			int fd=open(path,O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

			if(anchor[-1]=='2'){
				d_stderr=dup(STDERR_FILENO);
				dup2(fd,STDERR_FILENO);
				flag_err=1;
			}
			else{
				d_stdout=dup(STDOUT_FILENO);
				dup2(fd,STDOUT_FILENO);
				}
		}

		else{
			int fd=open(path,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

			if(anchor[-1]=='2'){
				d_stderr=dup(STDERR_FILENO);
				dup2(fd,STDERR_FILENO);
				flag_err=1;
			}

			else{
				d_stdout=dup(STDOUT_FILENO);
				dup2(fd,STDOUT_FILENO);
			}
		}	

		if(flag_err){
			len_cut++;
			str_cut(&(anchor[-1]),len_cut);
		}
		else{
			str_cut(anchor,len_cut);
		}

		/*WE LOWER THE FLAG*/

		flag=0;

	}while(anchor!=NULL);

	return save_pos;

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
	
	return redir_out(prompt);
	
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