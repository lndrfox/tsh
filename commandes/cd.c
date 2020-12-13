#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "tar_nav.h"


void cd (char * p){

	/*---IF WE HAVE NO ARGUMENT WE RETURN TO HOME---*/

	if(p==NULL){

		char * log = getlogin();
		char * home= malloc(strlen("/home/")+sizeof(char));
		home = strcpy(home,"/home/");
		home=realloc(home,strlen(home)+strlen(log)+sizeof(char));
		home=strcat(home,log);

		if(chdir(home)!=0){
			perror("chdir");
		}
		free(home);
		return;
	}

	/*---IF WE ARE ALREADY IN A TAR OR WANT TO ACCESS A TAR----*/

	else if(current_dir_is_tar() || string_contains_tar(p)){

		//CHECKING IF PATH IS VALID AND STORING IT

		char * a_path = path_is_valid(p);

		//IF THE PATH IS NOT VALID

		if(a_path==NULL){

				write(2,"tsh: cd: ",strlen("tsh: cd: "));
				write(2,p,strlen(p));
				write(2,": No such file or directory\n",strlen(": No such file or directory\n"));
				return;
		}

		/*IF WE EXIT THE TAR*/

		if(strcmp("",a_path)==0){

			setenv("tar",a_path,1);
			return;
		}

		/*WE COPY A_PATH SO WE CAN DECOMOOSE IT WITHOUT BREAKING IT*/

		char * a_path_copy=malloc(strlen(a_path)+sizeof(char));
		strcpy(a_path_copy,a_path);

		/*WE DECOMPOSE A_PATH_COPY*/

		char ** tokens_a_path=decompose(a_path_copy,"/");

		/*IN THAT CASE, WE ARE OUT OF THE TAR, BUT THERE IS
		STILL A PATH LEFT TO ACCESS*/

		if(!string_contains_tar(tokens_a_path[0])){

			/*WE RESET THE TAR ENV VAR OTHERWISE WE ARE STUCK IN A LOOP*/

			setenv("tar","",1);

			/*WE LOOP ON THE TOKENS OF A_PATH AND CALL CD ON EACH OF THEM
			ALLOWING US TO CHOOSE IF WE NEED TO USE THE TAR VERSION OF CD
			OR THE NORMAL VERSION*/

			int cpt=0;
			while(tokens_a_path[cpt]!=NULL){

				cd(tokens_a_path[cpt]);
				cpt++;
			}

			free(tokens_a_path);
			free(a_path_copy);
			free(a_path);
			return;
		}

		/*OTHERWISE WE SIMPLY UPDATE THE ENV VAR TAR*/

		else{
	
			setenv("tar",a_path,1);
			free(tokens_a_path);
			free(a_path_copy);
			free(a_path);
			return;
		}


	}

	/*-----IF WE ARE NOT DEALING WITH TAR FILES-----*/

	else{

		/*TOKING THE PATH*/

		char * p_copy=malloc(strlen(p)+sizeof(char));
		strcpy(p_copy,p);
		char ** tokens = decompose(p_copy,"/");

		/*THIS IS WHERE WE STORE THE PATH ONCE WE 
		HAVE DEALTH WITH ABSOLUTE PATH*/

		char * final;

		/*-----ABSOLUTE PATH------*/

		char c =p[0];

		/*IF THE FIRST CHARACTER IS A "/" THEN THIS IS AN ABSOLUTE PATH*/

		if(c == '/'){

			/*WE BUILD ABSOLUTE AS STRING CONTAINING
			A "/" AND THE FIRST TOKEN OF THE PATH*/

			char absolute [1+strlen(tokens[0])];
			strcat(absolute,"/");
			strcat(absolute,tokens[0]);

			/*ERROR MANAGMENT*/

			if(chdir(absolute)!=0){

				write(2,"tsh: cd: ",strlen("tsh: cd: "));
				write(2,p,strlen(p));
				write(2,": No such file or directory\n",strlen(": No such file or directory\n"));
				return;
			}

			/*SINCE IT WAS AN ABSOLUTE PATH, WE IGNORE THE FIRST TOKEN
			WHEN WE FLATTEN THE PATH*/

			final=flatten(&(tokens[1]),"/");
		}


		/*----RELATIVE PATH------*/

		/*WE FLATTEN ALL THE PATH*/

		else{

			final=flatten(tokens,"/");

		}

		/*ERROR MANAGMENT*/

		if(chdir(final)!=0){

			write(2,"tsh: cd: ",strlen("tsh: cd: "));
			write(2,p,strlen(p));
			write(2,": No such file or directory\n",strlen(": No such file or directory\n"));
			
				
		}

		free(tokens);
		free(p_copy);
		free(final);
		return;

	}

}