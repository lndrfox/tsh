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

void cat_tar (int fd,char * arg){

		struct posix_header hd;
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
				exit(-1);
			}

			//IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE GOOD HEADER

			if((hd.name[0]=='\0')){

				print_error("cat",arg,"No such file or directory");
				return ;
			}

			//READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER


			sscanf(hd.size, "%o",&size);

			//IF WE FOUND THE RIGHT HEADER, WE GET OUT OF THE LOOP
			
			if(strcmp(hd.name,arg)==0){

				break;
			}
	
			//OTHERWISE WE GET TO THE NEXT HEADER

			lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


		}while(strcmp(hd.name,arg)!=0);

		if(hd.typeflag=='1' ||  hd.typeflag=='2'){

			printf("%s\n",hd.linkname);
			int fd2= open(hd.linkname,O_RDONLY);

			if(fd2>=0){

				size_t size= lseek(fd2,0,SEEK_END);

				char * buffer [size];

				int readd=read(fd,buffer,size);

				if(readd<0){

					perror("read");
					exit(-1);
				}

				if(write(STDOUT_FILENO,buffer,size)<0){

					perror("write");
					exit(-1);
				}

				return;
			}

			else{

				char * linkname=malloc(sizeof(hd.linkname));
				strcpy(linkname,hd.linkname);

				lseek(fd,0,SEEK_SET);

				do{

			// READING AN HEADER

			int rdcount=read(fd,&hd,BLOCKSIZE);

			//ERROR MANAGMENT

			if(rdcount<0){

				perror("reading tar file");
				close(fd);
				exit(-1);
			}

			//IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE GOOD HEADER

			if((hd.name[0]=='\0')){

				print_error("cat",arg,"No such file or directory");
				return ;
			}

			//READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER


			sscanf(hd.size, "%o",&size);

			//IF WE FOUND THE RIGHT HEADER, WE GET OUT OF THE LOOP
			
			if(strcmp(hd.name,linkname)==0){

				break;
			}
	
			//OTHERWISE WE GET TO THE NEXT HEADER

			lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


		}while(strcmp(hd.name,linkname)!=0);

			}

			
		}

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



}
// TO USE : ./cat fichiertar.tar fichier autrefichier ...

int main(int argc, char *argv[]){

	// CHECKING IF WE HAVE ENOUGH ARGUMENTS

	if (argc<1){

		prints("Error invalid argument \n");
	}

  
   // WE LOOP ON EVERY ARGUMENT

  for (int i=1; i<argc;i++){

    
	//we get the tar to open and the path for the file
	//from tar_and_path
	char ** arg = tar_and_path(argv[i]);
 	char * tar = malloc(strlen(arg[0])+sizeof(char));
 	strcpy (tar,arg[0]);
 	char * path = malloc(strlen(arg[1])+sizeof(char));
 	strcpy (path,arg[1]);

    // OPENING THE TAR FILE
    int fd=open(tar,O_RDONLY);

    if(fd<0){

    	print_error("cat",argv[i],"No such file or directory");
    	return -1;
    }

	cat_tar(fd,path);


	}

	prints("\n");

	return 0;
}
