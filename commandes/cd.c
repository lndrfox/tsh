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

		return;
	}

	/*---IF WE ARE ALREADY IN A TAR OR WANT TO ACCESS A TAR----*/

	else if(current_dir_is_tar() || string_contains_tar(p)){

		/*BUILDING THE FULL TAR PATH, ACTUAL PATH IN TAR + ARG */

		char * path=(char * ) malloc(strlen(getenv("tar"))+sizeof(char));
		strcpy(path,getenv("tar"));
		path=realloc(path,strlen(path)+strlen("/")+sizeof(char));
		path=strcat(path,"/");
		path=realloc(path,strlen(path)+strlen(p)+sizeof(char));
		path=strcat(path,p);
		
		//CHECKING IF PATH IS VALID AND STORING IT

		path = path_is_valid(path);

		//IF THE PATH IS NOT VALID

		if(path==NULL){

				write(2,"Path invalid\n",strlen("Path invalid\n"));
		}

		else{
			
			setenv("tar",path,1);
		}

	}

	/*-----IF WE ARE NOT DEALING WITH TAR FILES-----*/

	else{


		/*TOKING THE PATH*/

		char ** tokens = decompose(p,"/");
		int cpt=0;

		/*-----ABSOLUTE PATH------*/

		char c =p[0];

		if(c == '/'){

			char absolute [1+strlen(tokens[cpt])];
			strcat(absolute,"/");
			strcat(absolute,tokens[cpt]);

			/*ERROR MANAGMENT*/

			if(chdir(absolute)!=0){

				perror("chdir");
			}

			cpt++;

		}


		/*----RELATIVE PATH------*/

		else{

			cpt=0;
		}

		/*WE ACCESS EACH TOKEN*/

		while(tokens[cpt]!=NULL){

			/*ERROR MANAGMENT*/

			if(chdir(tokens[cpt])!=0){

				perror("chdir");
				
			}

			cpt++;

		}


	}

	

	

}