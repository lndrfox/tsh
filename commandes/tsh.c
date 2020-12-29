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


char * get_line();
char * get_last_dir();
void exec_custom(char ** tokens,int boolean);
void str_cut(char *prompt ,int size);
char * redir_out(char * prompt);
char * redir_in(char * prompt);
void reinit_descriptors();
char * redir(char * prompt);
int goes_back_in_tar(char * path);
char ** split_args(char ** args);
void exec_tar_or_bin(char ** tokens, int boolean);
void exec_split(char ** tokens);
void exec_custom(char ** tokens,int boolean);



int run=1;
int d_stdout=1;
int d_stdin=0;
int d_stderr=2;
char path_home [PATH_MAX + 1];
char ** delete;
char * mv_out;
char * mv_err;

/*READS A LINE ENTERED IN THE TERMINAL AND RETURNS IT*/

char * get_line(){

	char * line = NULL;
	line= readline(" ");
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

		int flag_err=(anchor[-1]=='2'); //THIS FLAG IS RAISED IF WE NEED TO REDIRECT STDERR_FILENO

		/*IF THE GIVEN PATH DOESNT GOES BACK IN TAR
		WE SIMPLY APPLY TRUE PATH TO IT*/

		if(!goes_back_in_tar(path)){

			path=true_path(path);
		}

		/*IF IT IS INSIDE A TAR*/

		else{

			/*WE COPY THE FILE OUTSIDE OF THE TAR AND USE THE COPY
			FOR OUR OPEN THEN WE ADD IT TO THE DELETE ARRAY SO IT CAN BE DELETED LATER*/

			char *copy[4];
			copy[0]="cp";
			copy[1]=path;
			copy[3]=NULL;

			/*IF WHEN WE COPY WE GET -1 THEN THEN THERE WAS AN ERROR
			BUT IF IT'S -2 THEN IT JUST DOESNT EXIST YET IN THE TAR

			-1 WE RETURN ERROR

			-2 WE SIMPLY CONTINUE AS OPEN WILL CREATE THE FILE TO COPY ANYWAY
			SINCE IT HAS THE O_CREAT OPTION*/

			if(tar_vers_ext_cp(copy)==-1){

				char * err=malloc(strlen("error")+sizeof(char));
				if(err==NULL){

					perror("malloc");
					exit(-1);
				}
				strcpy(err,"error");
				return err;

			}


			/*WE COPY PATH IN MV_OUT/MV_ERR, AS WE WILL USE MV_OUT 
			IN THE FUNCTION REDIR TO COPY REDIR_OUT/REDIR_ERR PATH*/

			if(flag_err){

				mv_err=malloc(strlen(path)+sizeof(char));
				strcpy(mv_err,path);

			}

			else{

				mv_out=malloc(strlen(path)+sizeof(char));
				strcpy(mv_out,path);
			}

			/*WE CHANGE PATH TO REDIR_OUT/REDIR_ERR SO THAT THIS WILL THE NAME
			GIVEN TO OPEN*/

			if(flag_err){

				path=realloc(path,strlen("redir_err")+sizeof(char));
				strcpy(path,"redir_err");

			}
			
			else{

				path=realloc(path,strlen("redir_out")+sizeof(char));
				strcpy(path,"redir_out");
			}
			
			/*WE ADD REDIR_OUT/REDIR_ERR TO THE DELETE ARRAY SO THAT IT'S DELETED
			ONCE IT HAS BEEN COPIED IN THE TAR IN THE REDIR FUNCTION*/
			if(flag_err){

				if(delete[1]==NULL){

					delete[1]="redir_err";
				}

				else if(delete[1]!=NULL){

					if(strcmp(delete[1],"redir_err")!=0 && delete[2]==NULL){

						delete[2]="redir_err";

					}
					
				}
				
			}

			else{

				if(delete[1]==NULL){

					delete[1]="redir_out";
				}

				else if(delete[1]!=NULL){

					if(strcmp(delete[1],"redir_out")!=0 && delete[2]==NULL){

						delete[2]="redir_out";

					}
					
				}

			}		
		}

		/*------IF FLAG IS RAISED THEN WE HAVE EITHER >> OR 2>> SO THE
		DESCRIPTOR MUST BE OPENED IN APPEND MODE ------*/

		if(flag){

			/*WE OPEN THE DESCRIPTOR*/

			int fd=open(path,O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

			/*IF WE NEED TO REDIRECT STDERR_FILENO*/

			if(flag_err){
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

			if(flag_err){
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
		char * path_f=NULL;

		/*IF THE GIVEN PATH DOESNT GOES BACK IN TAR
		WE SIMPLY APPLY TRUE PATH TO IT*/

		if(!goes_back_in_tar(path)){

			path_f=true_path(path);
			
		}

		/*IF IT IS INSIDE A TAR*/

		else{

			/*WE COPY THE FILE OUTSIDE OF THE TAR AND USE THE COPY
			FOR OUR OPEN THEN WE ADD IT TO THE DELETE ARRAY SO IT CAN BE DELETED LATER*/

			char *copy[4];
			copy[0]="cp";
			copy[1]=path;
			copy[2]="redir_in";
			copy[3]=NULL;

			/*IN CASE OF MULTIPLE REDIRECTIONS, WE NEED TO REMOVE REDIR_IN SO WE
			CAN REPLACE IT WITH THE CONTENT OF THE LATEST FILE INDICATED*/

			if(access("redir_in",F_OK)==0){

				char *remove[3];
				remove[0]="rm";
				remove[1]="redir_in";
				remove[2]=NULL;

				exec_custom(remove,0);


			}

			/*IF THE COPY DIDN'T WORK IT MEANS THE FILE
			DOES NOT EXIT SO WE HANDLE THE ERROR AND RETURN "ERROR"*/

			if(tar_vers_ext_cp(copy)<0){

				print_error(NULL,path,"No such file or directory");

				char * err=malloc(strlen("error")+sizeof(char));
				if(err==NULL){

					perror("malloc");
					exit(-1);
				}
				strcpy(err,"error");
				return err;

			}

			/*WE CHANGE PATH TO REDIR_IN, THAT WAY A PATH NAMES REDIR_IN WILL BE
			CREATED IN THE CURRENT NON TAR DIRECTORY AND WILL BE OPENED, BECAUSE WE USED CP
			IT NOW CONTAINS WHAT WAS IN THE FILE INSIDE THE TAR THAT WE WANTED
			WE ADD IT TO THE DELETE ARRAY SO THAT IT WILL BE DELETED LATER ON*/
	
			path_f=malloc(strlen("redir_in")+sizeof(char));

			if(path_f==NULL){

				perror("malloc");
				exit(-1);
			}
			
			strcpy(path_f,"redir_in");

			if(delete[1]==NULL){

				delete[1]="redir_in";
			}
			
			else if(strcmp(delete[1],"redir_in")!=0 && delete[2]==NULL){
				delete[2]="redir_in";
			}

			else if(strcmp(delete[1],"redir_in")!=0 && 
				strcmp(delete[2],"redir_in")!=0 && delete[3]==NULL){

				delete[3]="redir_in";
			}

		}

			
		int fd=open(path_f,O_RDWR);

		if(fd<0){

			print_error(NULL,path_f,"No such file or directory");
			str_cut(anchor,len_cut);
			free(path);

			char * err=malloc(strlen("error")+sizeof(char));

				if(err==NULL){

					perror("malloc");
					exit(-1);
				}
				strcpy(err,"error");
				free(path);
				return err;
		}

		dup2(fd,STDIN_FILENO);

		/* WE CUT OUT THE < SYMBOLE + THE PATH*/

		str_cut(anchor,len_cut);
		free(path);
		free(path_f);

	}while(anchor!=NULL);

	return save_pos;
}

/*REINITIALIZE THE DESCRIPTORS THAT MIGHT HAVE BEEN CHANGED
BECAUS EOF REDIRECTIONS*/

void reinit_descriptors(){


	dup2(d_stdout,STDOUT_FILENO);
	dup2(d_stdin,STDIN_FILENO);
	dup2(d_stderr,STDERR_FILENO);


	/*IF MV_OUT ISNT NULL THEN THERE WAS
	AN OUT REDIRECTION AND WE COPY WHAT IS IN REDIR_OUT 
	TO THE MV_OUT SAVED PATH*/

	if(mv_out!=NULL){	

		/*WE CHECK IF THE FILE DID EXIST IN THE TAR BEFORE OR NOT
		IF YES WE REMOVE IT FROM THE TAR SO THAT IT'S CONTENT CAN BE REPLACED
		LATER ON BY WHAT IS IN REDIR_OUT/REDIR_ERR*/

		char ** tp=tar_and_path(mv_out);

		if(file_ndir_exists_in_tar(tp[1],tp[0])){

			char * rm[3];
			rm[0]="rm";
			rm[1]=mv_out;
			rm[2]=NULL;

			exec_custom(rm,1);
		}

		/*WE FREE TP AS WE DON'T NEED IT ANYMORE*/

		free(tp[0]);
		free(tp[1]);
		free(tp);

		char *copy[4];
		copy[0]="cp";
		copy[1]="redir_out";
		copy[2]=mv_out;
		copy[3]=NULL;

		ext_vers_tar_cp(copy);
	}

	/*IF MV_OUT ISNT NULL THEN THERE WAS
	AN ERROR REDIRECTION AND WE COPY WHAT IS IN REDIR_ERR
	TO THE MV_ERR SAVED PATH*/

	if(mv_err!=NULL){

		/*WE CHECK IF THE FILE DID EXIST IN THE TAR BEFORE OR NOT
		IF YES WE REMOVE IT FROM THE TAR SO THAT IT'S CONTENT CAN BE REPLACED
		LATER ON BY WHAT IS IN REDIR_OUT/REDIR_ERR*/

		char ** tp=tar_and_path(mv_err);

		if(file_ndir_exists_in_tar(tp[1],tp[0])){

			char * rm[3];
			rm[0]="rm";
			rm[1]=mv_err;
			rm[2]=NULL;

			exec_custom(rm,1);
		}

		/*WE FREE TP AS WE DON'T NEED IT ANYMORE*/

		free(tp[0]);
		free(tp[1]);
		free(tp);


		char *copy[4];
		copy[0]="cp";
		copy[1]="redir_err";
		copy[2]=mv_err;
		copy[3]=NULL;

		ext_vers_tar_cp(copy);
	}

	/*WE REMOVE ALL THE FILES LISTED
	IN DELETE*/

	if(delete[1]!=NULL){

		exec_custom(delete,0);
	}	

	/*WE FREE AND RETURN*/

	free(delete);
	free(mv_out);
	free(mv_err);

}

/*HANDLES REDIRECTIONS : CHANGES THE DESCRIPTORS SO SATISFY THE REDIRECTIONS 
SPECIFIED IN PROMPT*/

char * redir(char * prompt){

	/*WE INITIALIZE THE DELETE ARRAY
	CONTAINING ALL THE FILES CREATED FOR REDIRECTION PURPOSE
	THAT WILL NEED TO BE DELETED*/

	delete=calloc(5,sizeof(char *));
	delete[0]="rm";
	delete[1]=NULL;
	delete[2]=NULL;
	delete[3]=NULL;
	delete[4]=NULL;

	/*WE INITAILIZE MV_OUT AND MV_ERR AS NULL*/

	mv_out=NULL;
	mv_err=NULL;

	/*WE CALL REDIR_OUT THEN REDIR_IN ON THE RESULT
	OF REDIR_OUT BECAUSE IT REMOVES ANY POTENTION > REDIRECTIONS
	THAT REDIR_IN DOES NOT COMPUTE*/

	char * out =redir_out(prompt);
	char *ret =redir_in(out);
	free(out);


	return ret;
	
}

/*RETURNS 1 IF PATH ENDS IN A TAR DIRECTORY OR THE CHILD OF A TAR DIRECTORY, 0 OTHERWISE*/

int goes_back_in_tar(char * path){

	/*WE COPY THE PATH SO WE CAN DECOMPOSE IT*/

	char * path_copy=malloc(strlen(path)+sizeof(char));

	if(path_copy==NULL){

		perror("malloc");
		exit(-1);
	}

	strcpy(path_copy,path);

	char ** tokens=decompose(path_copy,"/");

	/*WE COPY GETENV("TAR") SO WE CAN DECOMPOSE IT*/

	char * get_env_copy=malloc(strlen(getenv("tar"))+sizeof(char));

	if(get_env_copy==NULL){

		perror("malloc");
		exit(-1);
	}

	strcpy(get_env_copy,getenv("tar"));

	char ** tokens_env =decompose(get_env_copy,"/");

	/*THIS CPT COUNTS HOW DEEP WE ARE IN THE TAR DIRECTORY*/

	int cpt_env=0;

	/*WE LOOP ON TOKENS_ENV TO GET THE CURRENT TAR DEPTH*/

	while(tokens_env[cpt_env]!=NULL){
		cpt_env++;
	}

	int cpt=0;//CPT FOR TOKENS
	int flag=0;// IS UP IF WE ENCOUNTERED A STRING WITH .TAR BUT NO .. RIGHT AFTER

	/*IF WE ARE CURRENTLY IN A TAR, WE SET THE FLAG AT 1 FOR CASES LIKE MKDIR REP*/

	if(current_dir_is_tar()){
		flag=1;
	}

	/*---- WE LOOP ON TOKENS ----*/

	while(tokens[cpt]!=NULL){

		/*IF WE ENCOUNTER A STRING CONTAINING .TAR WE RAISE THE FLAG*/

		if(string_contains_tar(tokens[cpt])){

			flag=1;
			cpt_env++;//WE ADD ONE TO THE TAR DEPTH

			/*IF THERE IS .. RIGHT AFTER THEN WE LOWER THE FLAG*/

			if(tokens[cpt+1]!=NULL){

				if(strcmp(tokens[cpt+1],"..")==0){
					flag=0;
				}		
			}
		}

		/*IF WE ENCOUNTER A .. AND I THE TAR DEPTH IS STILL >0 , WE DECREASE IT*/

		else if(strcmp(tokens[cpt],"..")==0){

			if(cpt_env>0){
				cpt_env--;
			}

		}

		/*IF WE ARE STILL IN A TAR AND MEET SOMETHING ELSE THAN A .. WE
		INCREASE THE TAR DEPTH*/

		if(flag==1 && strcmp(tokens[cpt],"..")!=0){

			cpt_env++;
		}

		/*IF THE TAR DEPTH IS NOT STRICTLY POSITIVE THEN WE LOWER THE FLAG*/

		if(cpt_env<=0){
			flag=0;
		}

		cpt++;
	}

	free(tokens);
	free(path_copy);
	free(tokens_env);
	free(get_env_copy);
	return flag;
}

/*GIVEN A CHAR ** ARGS STARTING WITH A COMMAND NAME, THEN ITS ARGUMENTS AND ENDING WITH NULL, 
SPLIT_ARGS REMOVES THE ARGUMENTS THAT ARE PATH OUTSIDE OF A TAR AND PUT THEM IN TOKEN_OUT_TAR
, A CHAR ** ALSO STARTING WITH THE COMMAND NAME AND ENDING BY NULL, AND THEN RETURNS IT*/

char ** split_args(char ** args){

	/*------ WE CREATE THE CHAR ** TOKEN_OUT_TAR, STARTING WITH
			ARGS[0] AND THEN A NULL TOKEN 						------*/

	int cpt_token_out_tar=0;

	/*WE USE I TO MAKE SURE WE ALSO COPY OPTIONS IF THERE IS ONE
	WE DON'T NEED TO SUPPORT MORE THAN ONE OPTION BECAUSE SPLIT_ARGS
	IS ONLY CALLED IF WE NEED TO HANDLE FUNCTIONS SUPPORTED FOR TAR FILES
	AND THOSE ONLY SUPPORT ONE OPTION AT BEST*/

	int i=2;

	if(args[1][0]=='-'){

		i++;
	}

	char ** token_out_tar= calloc(i,sizeof(char *));

	if(token_out_tar==NULL){

		perror("calloc");
		exit(-1);
	}

	char * copy_name=malloc(strlen(args[0])+sizeof(char));

	if(copy_name==NULL){

		perror("malloc");
		exit(-1);
	}

	strcpy(copy_name,args[0]);

	token_out_tar[cpt_token_out_tar]=copy_name;
	cpt_token_out_tar++;

	/*IF THERE IS AN OPTION WE COPY IT AS WELL*/

	if(	i==3){

		char * copy_op=malloc(strlen(args[1])+sizeof(char));

		if(copy_op == NULL){

			perror("malloc");
			exit(-1);
		}

		strcpy(copy_op,args[1]);

		token_out_tar[cpt_token_out_tar]=copy_op;
		cpt_token_out_tar++;
	}

	token_out_tar[cpt_token_out_tar]=NULL;
	int cpt=1;

	/*IF THERE IS AN OPTION WE INCREASE THE COUNTER TO IGNORE IT*/

	if(i==3){
		cpt++;
	}

	/*---- WE LOOP ON ARGS ----*/

	while(args[cpt]!=NULL){

		/*IF THE ARGUMENT SHOULDNT BE CALLED IN A TAR, 
		WE REMOVE IT FROM ARGS AND ADD IT TO TOKEN_OUT_TAR*/

		if(!(goes_back_in_tar(args[cpt]))){

			/*WE SAVE WHAT WE WANT TO COPY IN TOKEN_OUT_TAR*/

			char * save = malloc(strlen(args[cpt])+sizeof(char));

			if(save==NULL){

				perror("malloc");
				exit(-1);
			}

			strcpy(save,args[cpt]);

			/*WE PUT THE ARG WE WANT TO REMOVE AT NULL*/

			args[cpt]=NULL;

			/*WE SHIT THE WHOLE CHAR ** */

			int i=cpt+1;
			while(args[i]!=NULL){

				args[i-1]=args[i];
				i++;

			}

			/*WE PUT THE LAST ONE AS NULL AND WE DECREASE CPT
			AS ARGS IS NOW SHORTER*/

			args [i-1]=NULL;
			cpt --;

			/*WE ADD TRUE_PATH(SAVE) IN TOKEN OU TAR AND REALLOC ENOUGH SPACE
			*/
			
			char * tmp =true_path(save);
			free(save);

			/*IF TRUE PATH RETURNS "" IT MEANS WE JUST NEED TO WORK
			WITH THE CURRENT DIRECTORY ( OUTSIDE OF A TAR ) SO WE REPLACE IT
			BY A "."*/

			if(strcmp(tmp,"")==0){

				tmp=".";
			}

			token_out_tar[cpt_token_out_tar]=malloc(strlen(tmp)+sizeof(char));
			strcpy(token_out_tar[cpt_token_out_tar],tmp);

			if(strcmp(tmp,".")!=0){
				free(tmp);
			}

			cpt_token_out_tar++;

			token_out_tar=realloc(token_out_tar,(cpt_token_out_tar+1)*(sizeof(char *)));

		}

		cpt++;
	}

	/*WE MAKE SURE THAT TOKEN_OUT_TAR ENDS WITH NULL*/

	token_out_tar[cpt_token_out_tar]=NULL;

	return token_out_tar;

}

/*GIVEN TOKENS A CHAR ** CONTAINING A COMMAND NAMES AND IT'S ARGUMENT AND FINISH BY NULL,
AS WELL AS A BOOLEAN, EXEC_TAR_OR_BIN, CHECKS THAT THE TAR VERSION OR THE COMMAND EXISTS AND
EXEXCUTES IT IF BOOLEAN IS 1 AND EXACUTE THE COMMAND IGNORING TAR ELSE*/

void exec_tar_or_bin(char ** tokens, int boolean){

	//IF WE ARE WORKING WITH TAR FILES
	
	if(boolean){

		/*TO BE ABLE TO CALL THE COMMANDS FROM EVERYWHERE, WE BUILD TRUE_PATH
		AS A STRING CONTAINING PATH_HOME, THE DIRECTORY WHERE THE COMMAND FILES ARE
		THEN A / AND THEN TOKENS[0]*/

		char * true_path=malloc(strlen(path_home)+sizeof(char));
		true_path=strcpy(true_path,path_home);
		true_path=realloc(true_path,strlen(true_path)+2*(sizeof(char)));
		true_path=strcat(true_path,"/");
		true_path=realloc(true_path,strlen(true_path)+strlen(tokens[0])+sizeof(char));
		true_path=strcat(true_path,tokens[0]);

		//WE CHECK IF THE TAR COMMAND EXISTS/IS HANDLED

		if( access(true_path,F_OK|X_OK)==0){

			execv(true_path,tokens);				  		
		}

	}	
	execvp(tokens[0],tokens);
						  	 		
	print_error(NULL,tokens[0],"command not found");
}


/*SPLITS THE ARGUMENTS OF TOKEN IN TWO, A CHAR ** STARRTING WITH TOKENS[0]
AND THEN CONTAINING THE ARGUMENTS THAT ARE PATH INSIDE OF A TAR
THE OTHER STARTS THE SAME BUT CONTAIN THE ARGUMENTS TAR ARE A PATH OUTSIDE OF A TAR AND
THEN EXECUTE BOTH OF THEM*/

void exec_split(char ** tokens){

	if(strcmp(tokens[0],"error")==0){

		return;
	}

	if(strcmp(tokens[0],"tar")==0){

		exec_custom(tokens,0);
		return;
	}

	/*---- IF THERE IS NO ARGUMENT ----*/

	if(tokens[1]==NULL){

	/*IF THE CURRENT DIRECTORY IS A TAR*/	
	if(current_dir_is_tar() && (!strcmp(tokens[0],"cat")==0)){
			exec_custom(tokens,1);
		}

		/*IF THE CURRENT DIRECTORY IS NOT A TAR*/

		else{
			exec_custom(tokens,0);
		}
		
		return;
	}

	/*WE HANDLE OPTIONS HERE*/

	int cpt=1;
	int i=1;

	/*WE LOOP ON TOKENS*/

	while(tokens[i]!=NULL){

		/*IF THERE IS AN OPTION WE INCREMENT CPT*/

		if(tokens[i][0]=='-'){
			cpt++;

		/*IF THERE IS NO ARGUMENT AFTER THE OPTION, WE EXEC RIGHT AWAY*/

		if(tokens[i+1]==NULL){

			if(current_dir_is_tar()){

				exec_custom(tokens,1);
			}

			/*IF THE CURRENT DIRECTORY IS NOT A TAR*/

			else{
				exec_custom(tokens,0);
			}

			return;
			}
		}

		i++;
	}

	/*---- HANDELING CP AND MV ----*/

	if(strcmp(tokens[0],"mv")==0|| strcmp(tokens[0],"cp")==0){

		/*IF BOTH ARGS ARE OUT OF TAR*/
		if(tokens[cpt]!=NULL && tokens[cpt+1]!=NULL){

			/*COND IS TRUE IF WE WANT TO COPY A TAR FILE*/

			int cond = ((goes_back_in_tar(tokens[cpt]) && goes_back_in_tar(tokens[cpt+1])) &&
					strcmp(&(tokens[cpt][strlen(tokens[cpt])-4]),".tar")==0 
					&& strcmp(&(tokens[cpt+1][strlen(tokens[cpt+1])-4]),".tar")==0 );

			if((!goes_back_in_tar(tokens[cpt]) && !goes_back_in_tar(tokens[cpt+1])) || cond){

				/*IF COND IS TRUE BUT THE OPTION ISN'T -r THEN WE DON'T COPY BECAUSE
				.TAR FILES ARE CONSIDERED DIRECTORIES*/

				if(cond && strcmp(tokens[cpt-1],"-r")!=0){

					print_error(NULL,tokens[cpt],"can't copy a directory -r non specified");
					return;
				}

				tokens[cpt]=true_path(tokens[cpt]);
				cpt++;
				tokens[cpt]=true_path(tokens[cpt]);
				exec_custom(tokens,0);

				return;
			}

			exec_custom(tokens,1);

			return;
		}
	}

	/*-----SPLLITING THE ARGUMENTS, ARGS CONTAINS IN TAR ARG, OUT_TAR CONTAINS OUT TAR ARGUMENTS ----*/

	char ** out_tar=split_args(tokens);

	/*IF TOKENS HAS NO ARGUMENTS*/

	if(tokens[cpt]==NULL){

		exec_custom(out_tar,0);
	}

	/*IF OUT_TAR HAS NO ARGUMENTS*/

	else if(out_tar[cpt]==NULL){
		
		exec_custom(tokens,1);
	}

	/*IF THEY BOTH HAVE ARGUMENTS*/

	else{

		exec_custom(tokens,1);
		exec_custom(out_tar,0);
	}

	int j=0;
	while(out_tar[j]!=NULL){

		free(out_tar[j]);
		j++;
	}

	free(out_tar);

}

/*GIVEN A CHAR ** TOKENS CONTAINING A COMMAND NAMES, ITS ARGUMENTS AND FINISHING BY NULL
EXEC FORKS AND THE CHILD PROCESS EXECUTES THE COMMAND IN TOKENS*/


void exec_custom(char ** tokens,int boolean){

	/*WE FORK TO GET A NEW PROCESS*/
	int r,w;
	r=fork();

	switch(r){

		case -1: //ERROR

			perror("fork");
			exit(EXIT_FAILURE);

		case 0: //SON
			exec_tar_or_bin(tokens,boolean);
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
					exec_split(token_1);
					exit(EXIT_FAILURE);

				default: //FATHER, READER

					close(fd[1]);
					dup2(fd[0],STDIN_FILENO);

					/*IF THERE IS NOTHING ELSE IN NEXT, THEN WE SIMPLY 
					EXEC THE COMMAND*/

					if(next[1]==NULL){

						exec_split(token_2);
					}

					/*ELSE WE DECOMPOSE THE COMMAND IN NEXT[1] AND
					THEN WE CALL EXEC_PIPES RECURSIVELY WITH TOKEN_2, TOKEN_NEXT
					AND NEXT[1] FOR THE ARGUMENT NEXT*/

					else{

						char * token_cpy= malloc(strlen(next[1])+sizeof(char));

						if(token_cpy==NULL){

							perror("malloc");
							exit(-1);
						}

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

	if(prompt_cpy==NULL){

		perror("malloc");
		exit(-1);
	}

	strcpy(prompt_cpy,prompt);

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

			exec_split(tokens);

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