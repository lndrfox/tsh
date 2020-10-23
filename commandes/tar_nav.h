#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>	
#include <unistd.h>
#include <limits.h>
#include "tar.h"

char ** decompose(char * prompt, char * delimiter);
int string_contains_tar(char * string);
int current_dir_is_tar();
int tar_file_exists(char * path, char * tar);
char * get_tar_name();
char * flatten(char ** tokens, char * delimiter);
char * path_is_valid(char * path);
int file_exists_in_tar(char * path, char *  tar);


/*DECOMPOSE THE STRING PROMPT ACCORDING TO THE STRING DELIMITER AND RETURNS THE TOKENS
IN CHAR ** TOKEN ENDING BY NULL*/

char ** decompose(char * prompt, char * delimiter){

	/*WE CALCULATE HOW MANY TOKENS THE STRING HAS*/
	char * prompt_cpy=(char * ) malloc(strlen(prompt)+sizeof(char));
	strcpy(prompt_cpy,prompt);
	//memcpy(prompt_cpy,prompt,sizeof(prompt)+sizeof(char));

	char * check_len = strtok(prompt_cpy, delimiter);

	int i =1;

	while((check_len =strtok(NULL, delimiter) ) !=NULL){

		i++;

	}

	/* THIS WERE WE STORE ALL THE TOKENS*/
	char ** tokens= (char **) calloc(i+1,sizeof(char *));

	/*ERROR MANAGEMENT*/

	if(tokens ==NULL){

		exit (-1);
	}

	/*WE NEED TO STORE THE FIRST RESULT OF STRTOK TO NOT LOSE ANYTHING*/
	char * token = strtok(prompt, delimiter);
	int cpt_tokens=0;
	tokens[cpt_tokens]=token;
	cpt_tokens++;

	/*AS LONG AS THERE IS SOMETHING TO DECOMPOSE WE TOK AND STORE IT IN TOKENS*/

	while((token =strtok(NULL, delimiter) ) !=NULL){

		tokens[cpt_tokens]= token;
		cpt_tokens++;

	}

	/*WE ADD NULL AT THE END OF TOKENS */

	tokens[cpt_tokens]=token;
	return tokens;

}


/*CHECKS IF THE STRING STRING IS A TAR PATH */

int string_contains_tar(char * string){

	return strstr(string,".tar")!=NULL;
}

/*CHECK IF THE CURRENT WORKING DIRECTORY IS INSIDE OF A TAR*/

int current_dir_is_tar(){

	if( getenv("tar")!=NULL ){

		if(strcmp(getenv("tar"),"")!=0){

			return 1;
		}

	}

	return 0;

}

/*CHECKS IF THE DIRECTORY PATH CONTAINS THE TAR FILE TAR*/

int tar_file_exists(char * path, char * tar){

	/*OPENING THE DIR PATH*/

	DIR *dirp = opendir(path);

	/*ERROR MANAGMENT*/

	if(dirp==NULL){

		perror("opendir");
		exit(-1);
	}

	struct dirent *entry;

	/*LOOPING ON THE DIRECTORY*/

	while((entry = readdir(dirp))){

		/*WE MAKE SURE TO IGNORE SPECIAL ENTRIES*/

		if(strcmp(".",entry->d_name)!=0 && strcmp("..",entry->d_name)!=0) {

			/*WE CHECK IF THE CURRENT ENTRY IS A TAR*/

			if(string_contains_tar(entry->d_name)){

				/*WE CHECK IF IT IS SPECFICIALLY TAR*/

				if (strcmp(entry->d_name,tar)==0){

					//free(entry);
					return 1;	
				}
			}
		}	
	}

	//free(entry);
	return 0;

}

/*RETURNS THE NAME OF THE TAR FILE WE ARE CURRENTLY WORKING ON*/

char * get_tar_name(){

	if(!current_dir_is_tar()){

		return NULL;
	}

	char ** tokens = decompose(getenv("tar"),"/");
	return tokens[0];

}

/*	CAT THE STRINGS IN TOKENS WITH DELIMITER BETWEEN THEM INTO AN ONLY STRIN
EG IF TOKEN IS "a" "b" AND "c" AND DELIMITERS "/" IT RETURNS "a/b/c"*/

char * flatten(char ** tokens, char * delimiter){

	if(tokens[1]==NULL){

		return "";
	}

	char *ret = malloc(strlen(tokens[0]));
	memset(ret,0,strlen(tokens[0]));
	ret=strcat(ret,tokens[1]);

	int cpt=2;


	while(tokens[cpt]!=NULL){

		ret =realloc(ret,strlen(ret)+1+strlen(tokens[cpt]));
		ret=strcat(ret,delimiter);
		ret=strcat(ret,tokens[cpt]);

		cpt++;

	}

	return ret;


}

int file_exists_in_tar(char * path, char * tar){


	// OPENING THE TAR FILE

	int fd=open(tar,O_RDONLY);

	//ERROR MANAGMENT

	if(fd==-1){
		perror("open tar file");
		exit(-1);
	}

	// THIS IS WERE WE STORE HEADERS

	struct posix_header hd;

		do{

			// READING AN HEADER

			int rdcount=read(fd,&hd,BLOCKSIZE);

			//ERROR MANAGMENT

			if(rdcount<0){

				perror("reading tar file");
				close(fd);
				return -1;
			}


			//IF WE FOUND THE HEADER

			if(strcmp(hd.name,path)==0 && hd.typeflag=='5'){

				return 1;

			}

			
			//READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			unsigned int size;
			sscanf(hd.size, "%o",&size);

			//WE GET TO THE NEXT HEADER

			lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


		}while(hd.name[0]!='\0');

		return 0;

}



/*IF PATH IS A VALID PATH, RETURN A CHAR * WITH THE PATH ( WITH .. PROCESSED ) ELSE
RETURNS NULL*/

char * path_is_valid(char * path){

	return ("c.tar");

	char ** tokens =decompose(path,"/");
	char bufdir [PATH_MAX + 1];
	getcwd(bufdir,sizeof(bufdir));

	/*	CHECKS THAT THE TAR FILE THE PATH IS IN IS IN THE CURRENT LAST NOT TAR DIRECTORY*/

	if(!tar_file_exists(bufdir,tokens[0])){

		return NULL;
	}

	/*IF THERE IS NOTHING ELSE IN TOKEN THEN WE ARE DONE*/

	if(tokens[1]==NULL){

		return path;
	}

	int i=0;

	/*WE LOOP ON ALL THE TOKENS TO COUNT HOW MANY TOKENS THE FINAL PATH HAS*/

	int cpt_tok=1;

	while(tokens[cpt_tok]!=NULL){

		/*HANDLING THE .. CASE*/

		if(strcmp(tokens[cpt_tok],"..")==0){

			/*IF THERE IS NOTHING BEFORE THE .. THERE IS AN ERROR*/

			if(i ==0){

				return NULL;
			}

			else{

				i --;
			}

		}

		/*ANY OTHER CASES*/

		else{

			i ++;
		}

		cpt_tok++;
	}

	/*F_PATH CONTAINS THE TOKENS OF THE TRUE PATH WITHOUT ..*/

	char ** f_path =(char **) calloc(i+2,sizeof(char *));

	if(f_path==NULL){

		exit(-1);
	}

	f_path[0]=tokens[0];

	/*CPT IS THE NUMBER OF THE TOKEN WE ARE CURRENTLY ACCESSING IN F_PATH*/

	int cpt=1;

	/*WE LOOP ON ALL THE TOKENS */

	cpt_tok=1;

	while(tokens[cpt_tok]!=NULL){



		/*HANDLING THE .. CASE*/

		if(strcmp(tokens[cpt_tok],"..")==0){

			/*CASE WERE WE GET OUT OF THE TAR*/

			if(i ==1){

				return "";
			}

			else{

			cpt --;
			f_path[cpt]=NULL;

			}

		}

		/*ANY OTHER CASES*/

		else{
			f_path[cpt]=tokens[cpt_tok];
			cpt ++;
		}

		cpt_tok++;
	}

	f_path[cpt]=NULL;

	/*FLATTENING THE PATH INTO A STRING*/

	char * pathf =flatten(f_path, "/");

	/*IF SUCH A DIRECTORY EXISTS IN THE TAR THEN WE CAN RETURN PATH*/

	/*WE NEED TO ADD A / AT THE END TO SEARCH IN THE TAR*/

	char * pathtest=(char * ) malloc(sizeof(pathf)+sizeof(char));
	memcpy(pathtest,pathf,strlen(pathf)+sizeof(char));
	pathtest=realloc(pathtest,sizeof(pathtest)+strlen("/"));
	pathtest=strcat(pathtest,"/");


	if(file_exists_in_tar(pathtest,tokens[0])){

		/*BUILDING THE STRING TO HAVE THE FORMAT "/PATH/PATH/../" */

		char * final = (char * ) malloc(sizeof(tokens[0])+sizeof(char));
		memcpy(final,tokens[0],strlen(tokens[0])+sizeof(char));
		final=realloc(final,sizeof(final)+strlen("/"));
		final=strcat(final,"/");
		final=realloc(final,sizeof(final)+strlen(pathf));
		final=strcat(final,pathf);

		return final;
	}

	return NULL;
}
