#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "tar.h"
#include "tar_nav.h"
#include "print.h"


// TO USE : ./cat fichiertar.tar fichier autrefichier ...

int main(int argc, char *argv[]){

	// CHECKING IF WE HAVE ENOUGH ARGUMENTS

	if (argc<1){

		prints("Error invalid argument \n");
	}

        // WE LOOP ON EVERY DIRECTORY THAT WE NEED TO CREATE
  int fd;
  for (int i=1; i<argc;i++){

    struct posix_header hd;

		//we get the tar to open and the path for the file
		//from tar_and_path
		char ** arg = tar_and_path(argv[i]);
 	 char * tar = malloc(strlen(arg[0])+sizeof(char));
 	 strcpy (tar,arg[0]);
 	 char * path = malloc(strlen(arg[1])+sizeof(char));
 	 strcpy (path,arg[1]);

    // OPENING THE TAR FILE
    fd=open(tar,O_RDWR);

		unsigned int size;


		// THIS LOOP ALLOWS US TO LOOK FOR THE HEADER CORRESPONDING TO THE FILE WE WANT TO
		// FIND IN THE TAR


		do{

			// READING AN HEADER

			int rdcount=read(fd,&hd,BLOCKSIZE);

			//ERROR MANAGMENT

			if(rdcount<0){

				perror("reading tar file");
				close(fd);
				return -1;
			}

			//IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE GOOD HEADER

			if((hd.name[0]=='\0')){

				prints("Error, file not found int the .tar\n");
				return -1;
			}

			//READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER


			sscanf(hd.size, "%o",&size);

			//IF WE FOUND THE RIGHT HEADER, WE GET OUT OF THE LOOP
			if(strcmp(getenv("tar"),tar)==0){
				if(strcmp(hd.name,path)==0){

					break;
				}
			}

			else{

				char *relative = get_path_without_tar();
				relative= realloc(relative,strlen(relative)+strlen(hd.name)+sizeof(char));
				relative=strcat(relative,hd.name);

				if(strcmp(relative,path)==0){

					break;
				}


			}


			//OTHERWISE WE GET TO THE NEXT HEADER

			lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


		}while(strcmp(hd.name,argv[i])!=0);

		//BUFFER TO READ THE CONTENT OF THE FILE

		char buff [BLOCKSIZE];

		prints("\n");

		//WE READ THE CONTENT OF THE FILE AND THEN WRITE IT IN THE STDOUT

		for(unsigned int i=0; i<((size+ BLOCKSIZE - 1) >> BLOCKBITS); i++){

			// READING THE BLOCK

			int rdtmp = read(fd, buff, BLOCKSIZE);

			//EROR MANAGMENT

			if(rdtmp<0){

				perror("Reading tar file");
				exit(-1);
			}

			//WRITING THE BLOCK AND ERROR MANGEMENT

			if(write(STDOUT_FILENO,buff, rdtmp)<0){

				perror("Writing file content");
				exit(-1);

			}

			//RESETING THE BUFFER

			memset(buff, 0, BLOCKSIZE);
		}

		//WE GO BACK TO THE BEGINNING OF THE TAR FILE TO MAKE SURE WE DONT MISS
		// THE HEADER WE WILL LOOK FOR

		lseek(fd,0,SEEK_SET);


	}
	prints("\n");

	close(fd);



	return 0;
}
