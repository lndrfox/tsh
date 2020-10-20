#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <dirent.h>	
#include <unistd.h>
#include <limits.h>
#include "tar.h"

char ** decompose(char * prompt, char * delimiter);
int string_contains_tar(char * string);
int current_dir_is_tar();
int tar_file_exists(char * path, char * tar);
char * get_tar_name();
char * get_path_in_tar();
int path_is_valid(char * path);


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

		tokens =realloc(tokens, sizeof(tokens)+sizeof(char* ));
		tokens[cpt_tokens]= token;	
		cpt_tokens++;

	}

	/*WE ADD NULL AT THE END OF TOKENS */
	tokens =realloc(tokens, sizeof(tokens)+sizeof(char* ));
	tokens[cpt_tokens]=NULL;

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

					free(entry);
					return 1;	
				}
			}
		}	
	}

	free(entry);
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

/*RETURNS THE CURRENT PATH INSIDE OF THE TAR*/

/*char * get_path_in_tar(){

	if(!current_dir_is_tar()){

		return NULL;
	}

	char ** tokens = decompose(getenv("tar"),"/");

	char * cat= malloc(sizeof(getenv("tar"))-sizeof(tokens[0]));

	for(int i=1;i<(sizeof(tokens)/sizeof(char *))){

		cat = strcat(cat,tokens[i]);
	}

	return cat;

}

char * path_is_valid(char * path){

	char ** tokens =decompose(path,"/");

	char bufdir [PATH_MAX + 1];
	getcwd(bufdir,sizeof(bufdir));


	if(!tar_file_exists(bufdir,path[0])){

		return NULL;
	}

	if(tokens[1]==NULL){

		return path;
	}

	char * f_path = malloc(strlen(tokens[1]));

	if(f_path==NULL){

		exit(-1);
	}

	for(int i=1; i)
}*/
