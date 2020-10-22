#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


void cd (char * p){

	if(current_dir_is_tar() || string_contains_tar(p)){

			/*BUILDING THE FULL TAR PATH, ACTUAL PATH IN TAR + ARG */

		char * path = malloc(strlen(p)+strlen(getenv("tar"))+1);
		memset(path,0,strlen(path));
		path = strcat(path,getenv("tar"));
		path=strcat(path,"/");
		path=strcat(path,p);

		/*CHECKING IF PATH IS VALID AND STORING IT*/

		path = path_is_valid(path);

		/*IF THE PATH IS NOT VALID*/

		if(path==NULL){


		}

		/*IF THE PATH IS VALID WE UPDATE IT*/

		else{

			setenv("tar",path,1);
		}
	}

	else{


	}

	

	

}