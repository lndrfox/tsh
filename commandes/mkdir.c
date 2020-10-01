#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <grp.h>
#include <time.h>
#include "tar.h"


// TO USE : ./cat fichiertar.tar fichier autrefichier ...

int main(int argc, char *argv[]){

	// CHECKING IF WE HAVE ENOUGH ARGUMENTS

	if (argc<2){

		printf("Error invalid argument \n");
		return -1;
	}

	// OPENING THE TAR FILE

	int fd=open(argv[1],O_RDWR);

	//ERROR MANAGMENT

	if(fd==-1){
		perror("open tar file");
		exit(-1);
	}

	// THIS IS WERE WE STORE HEADERS

	struct posix_header hd;

		// THIS LOOP ALLOWS US TO LOOK FOR THE HEADER CORRESPONDING TO THE FILE WE WANT TO
		// FIND IN THE TAR

	for (int i=2; i<argc;i++){

		if(sizeof(argv[i])>sizeof(char[100])){

			printf("Errorn directory %s name is too long", argv[i]);
			exit(-1);

		}



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

				break;
			}

			//IF WE FOUND THE RIGHT HEADER, WE GET OUT OF THE LOOP

			if(strcmp(hd.name,argv[i])==0){

				printf ( "Error, the directory %s already exists", argv[i]);
				exit(-1);

			}

			
			//READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			unsigned int size;
			sscanf(hd.size, "%o",&size);

			//OTHERWISE WE GET TO THE NEXT HEADER

			lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


		}while(strcmp(hd.name,argv[i])!=0);


		struct posix_header dir;
		memset(&dir,0,BLOCKSIZE);

		char name_tmp[100];
		sprintf(name_tmp, argv[i]);
		char name_tmp2 [100]="/";

		strcat(name_tmp, name_tmp2);

		sprintf(dir.name,name_tmp);
		sprintf(dir.mode,"0000700");
		sprintf(dir.size,"%011o",0);
		sprintf(dir.magic,TMAGIC);
		long t_acc= time(NULL);
		sprintf(dir.mtime ,"%ld", t_acc);
		dir.typeflag='5';
		sprintf(dir.version,TVERSION);
		getlogin_r(dir.uname, sizeof(dir.uname));

		gid_t gid= getgid();
		struct group * g= getgrgid(gid);

		if(g==NULL){
			perror("Reading group ID");
			exit(-1);
		}
		//memset(dir.prefix,0,sizeof(dir.prefix));
		//memset(dir.junk,0, sizeof(dir.junk));

		sprintf(dir.gname,g->gr_name);
		set_checksum(&dir);

		lseek(fd,-512, SEEK_CUR);

		int rdd=write(fd,&dir,sizeof(dir));

		if(rdd<BLOCKSIZE){

			perror("Error writing in file");
			exit(-1);
		}

		char buff [BLOCKSIZE];
		memset(buff,0,BLOCKSIZE);

		for(int i=0;i<2;i++){

			int rdd=write(fd,buff,BLOCKSIZE);

			if(rdd<BLOCKSIZE){

				perror("Error writing in file");
				exit(-1);
			}

		}




		//WE GO BACK TO THE BEGINNING OF THE TAR FILE TO MAKE SURE WE DONT MISS
		// THE HEADER WE WILL LOOK FOR

		lseek(fd,0,SEEK_SET);

	}


	

	close(fd);



	return 0;
}