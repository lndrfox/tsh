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
#include "tar_nav.h"



/*DECOMPOSE THE STRING PROMPT ACCORDING TO THE STRING DELIMITER AND RETURNS THE TOKENS
IN CHAR ** TOKEN ENDING BY NULL*/

char ** decompose(char * prompt, char * delimiter){

	/*WE CALCULATE HOW MANY TOKENS THE STRING HAS*/
	char * prompt_cpy=(char * ) malloc(strlen(prompt)+sizeof(char));
	strcpy(prompt_cpy,prompt);

	char * check_len = strtok(prompt_cpy, delimiter);

	int i =1;

	while((check_len =strtok(NULL, delimiter) ) !=NULL){

		i++;

	}

	free(check_len);
	free(prompt_cpy);

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

	free(token);
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

					free(dirp);
					return 1;	
				}
			}
		}	
	}

	free(dirp);
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

	char *ret = malloc(strlen(tokens[0])+sizeof(char));
	strcpy(ret,tokens[0]);

	int cpt=1;

	while(tokens[cpt]!=NULL){

		ret =realloc(ret,strlen(ret)+(2*sizeof(char)));
		ret=strcat(ret,delimiter);
		ret=realloc(ret,strlen(ret)+strlen(tokens[cpt])+sizeof(char));
		ret=strcat(ret,tokens[cpt]);

		cpt++;

	}

	return ret;


}

/*RETURN THE ACTUAL PATH IN THE TAR BUT WITHOUT THE NAME OF THE TAR FILE AT THE BEGGINIGN*/

char * get_path_without_tar(){

	if(!current_dir_is_tar()){

		return NULL;
	}

	char ** tokens = decompose(getenv("tar"),"/");
	return flatten(&tokens[1],"/");
}

/*CHECKS IF THE FILE PATH EXIST IN THE TAR FILE TAR*/

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
RETURNS NULL, IF WE SIMPLY NEED TO EXIT THE TAR, RETURNS ""*/

char * path_is_valid(char * path){

	/*WE COPY THE PATH TO AVOID BREAKING IT*/

	char* path_cpy= malloc(strlen(path)+sizeof(char));
	strcpy(path_cpy,path);

	/*WE DECOMPOSE THE PATH*/

	char ** tokens = decompose(path_cpy,"/");

	/*TRUE PATH WILL GIVE US 

	A - IF WE STAY IN THE TAR, A PATH LOOKING LIKE getenv("tar")+PATH PROCESSED
	WITHOUT ANY ..

	B - IF WE GET OUT OF THE TAR AND NOTHING ELSE IT WILL RETURN "exit"

	C - IF WE NEED TO GET OUT OF THE TAR BUT THERE IS STILL A PATH TO PROCESS AFTER THAT
	IT WILL RETURN THE PATH TO PROCESS AFTER GETTING OUT OF THE TAR, .. INCLUDED

	D - IF THE PATH IS NOT VALID IT RETURNS NULL*/

	char * pathf=true_path(path);

	/*D - IF PATHF IS NULL WE SIMPLY RETURN IT AS IT MEANS THE PATH
	ISN'T VALID*/

	if(pathf==NULL){

		return pathf;
	}

	/*WE COPY PATHF AND THEN DECOMPOSE IT*/

	char * pathf_copy= malloc(strlen(pathf)+sizeof(char));
	strcpy(pathf_copy,pathf);
	char ** tokens_pathf=decompose(pathf_copy,"/");

	/*B - IF WE HAVE EXIT THEN WE SIMPLY EXIT THE TAR*/

	if(strcmp(pathf,"exit")==0){

		return "";
	}

	/*C - WE STILL HAVE A PATH TO PROCESS BUT NOT IN THE TAR
	SO WE RETURN PATHF RIGHT AWAY*/

	if(!string_contains_tar(tokens_pathf[0])){

		return pathf;
	}

	/*CHECKS THAT THE TAR FILE THE PATH IS IN IS IN THE CURRENT LAST NOT TAR DIRECTORY*/	

	char bufdir [PATH_MAX + 1];
	getcwd(bufdir,sizeof(bufdir));	

	if(!tar_file_exists(bufdir,tokens_pathf[0])){

		return NULL;
	}

	/*IF THERE IS NO MORE TOKENS, AS WE JUST CHECKED THE EXISTENCE
	OF THE .TAR, WE CAN RETURN PATHF*/

	if(tokens_pathf[1]==NULL){

		return pathf;
	}

	/*WE FLATTEN AND THEN MODIFY THE STRING SO WE CAN GET PATHF BUT WITHOUT 
	THE XX.TAR TOKEN AT THE BEGGINING*/
	
	char * path_without_tar= flatten(&(tokens_pathf[1]),"/");
	path_without_tar=realloc(path_without_tar,strlen(path_without_tar)+2*sizeof(char));
	path_without_tar=strcat(path_without_tar,"/");;

	/*WE CHECK THAT THE PATH WE OBTAINED DOES EXIST IN THE TAR
	WE ARE CURRENTLY IN*/

	if(file_exists_in_tar(path_without_tar,tokens_pathf[0])){

		free(tokens);
		free(tokens_pathf);
		free(path_without_tar);
		free(pathf_copy);

		return pathf;
	}

	/*IF NOTHING FITS THEN WE RETURN NULL*/

	free(tokens);
	free(pathf);
	free(tokens_pathf);
	free(path_without_tar);
	free(pathf_copy);

	return NULL;
}



char * true_path(char * path){

  char * tar= malloc(strlen(getenv("tar"))+sizeof(char));
  strcpy(tar,getenv("tar"));

  char ** tokens = decompose(tar,"/");

  char * ar =malloc(strlen(path)+sizeof(char));
  strcpy(ar, path);

  char ** tokens2 = decompose(ar,"/");

  int i = 0; 
  int i2 = 0;
  
  while (tokens[i] != NULL){

    i++;

  }
  
  while (tokens2[i2] != NULL){

    if(strcmp(tokens2[i2],"..") == 0){

      if(i == 0||strcmp(tokens[i-1],"..")==0) {

      		tokens=realloc(tokens,(i+1)*sizeof(char *));
      		tokens[i]=tokens2[i2];
      		i++;
       }

      else{

      	tokens[i-1]=NULL;
      	i--;
      	tokens=realloc(tokens,i*sizeof(char *));

      }
           
    }

    else{

      tokens=realloc(tokens,(i+1)*sizeof(char *));
      tokens[i] = tokens2[i2];

      i++;
    }

    i2++;
  }

  tokens=realloc(tokens,(i+1)*sizeof(char *));
  tokens[i] = NULL;

  if(i==0){
  	return "exit";
  }

  char *ret = flatten(tokens,"/");
  return ret;
}

