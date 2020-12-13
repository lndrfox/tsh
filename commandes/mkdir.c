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
#include "print.h"
#include "tar_nav.h"


void create_dir(int fd, char *  arg, char * path){

	struct posix_header hd;

		do{

			// READING AN HEADER

			int rdcount=read(fd,&hd,BLOCKSIZE);

			//ERROR MANAGMENT

			if(rdcount<0){

				perror("reading tar file");
				close(fd);
				exit (-1);
			}

			//IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE HEADER THEN IT DOESNT EXIST AND WE CAN CREATE IT

			if((hd.name[0]=='\0')){

				break;
			}

			//IF WE FOUND THE HEADER, IT ALREADY EXISTS AND WE HAVE AN ERROR

			if(strcmp(hd.name,path)==0){

				prints("Error, the directory ");
				prints(path);
				prints(" already exists");
				exit(-1);

			}

			
			//READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			unsigned int size;
			sscanf(hd.size, "%o",&size);

			//WE GET TO THE NEXT HEADER

			lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


		}while(strcmp(hd.name,arg)!=0);

		//CREATING THE HEADER FOR THE NEW DIRECTORY


		struct posix_header dir;

		//INIT STRUCT MEMORY

		memset(&dir,0,BLOCKSIZE);

		//  WE NEED TO ADD / AT THE END OF THE DIRECTORY NAME FOR IT TO BE VALID

		char name_tmp[100];
		sprintf(name_tmp, "%s", path);
		char name_tmp2 [100]="/";
		strcat(name_tmp, name_tmp2);
		sprintf(dir.name, "%s", name_tmp);

		//FILLING MODE 


		sprintf(dir.mode,"0000700");

		//SIZE IS 0

		sprintf(dir.size,"%011o",0);


		//FILLING MAGIC FIELD


		sprintf(dir.magic,TMAGIC);

		// LAST MODIFICATION DATE

		unsigned int  t_acc= time(NULL);
		sprintf(dir.mtime ,"%o", t_acc);

		//DIRECTORY SO TYPE FLAG IS 5

		dir.typeflag='5';

		// VERSION

		sprintf(dir.version,TVERSION);

		//USER NAME

		getlogin_r(dir.uname, sizeof(dir.uname));

		//GETTING GROUP NAME

		gid_t gid= getgid();
		struct group * g= getgrgid(gid);

		//ERROR MANAGMENT

		if(g==NULL){
			perror("Reading group ID");
			exit(-1);
		}
		sprintf(dir.gname, "%s", g->gr_name);

		//SETTING CHECKSUM ONCE ALL THE OTHER FIELDS ARE FILLED

		set_checksum(&dir);
		
		// SINCE WE READ THE FIRST EMPTY BLOCK (TAR ENDS WITH 2 EMPTY BLOCKS) TO CHECK IF WE HAD READ ALL OF THE HEADERS
		// WE NEED TO GO BACK ONE BLOCK

		lseek(fd,-512, SEEK_CUR);

		//WRITING THE NEW DIRECTORY AT THE END OF THE FILE

		int rdd=write(fd,&dir,sizeof(dir));

		//ERROR MANAGEMENT

		if(rdd<BLOCKSIZE){

			perror("Error writing in file");
			exit(-1);
		}

		//WE HAD THE TWO MANDATORY EMPTY BLOCKS AT THE END OF THE TAR FILE

		char buff [BLOCKSIZE];
		memset(buff,0,BLOCKSIZE);

		for(int i=0;i<2;i++){

			int rdd=write(fd,buff,BLOCKSIZE);

			if(rdd<BLOCKSIZE){

				perror("Error writing in file");
				exit(-1);
			}

		}
}



int main(int argc, char *argv[]){    

  // WE LOOP ON EVERY DIRECTORY THAT WE NEED TO CREATE
 
  for (int i=1; i<argc;i++){

	  
    char *test;
    test=argv[i];

    //We set a path removing every .. for argv1
    char path1[100]; 
    strcpy(path1,true_path(test));

    //Will be a counter 
    int i2 = 0;

    //Array of the decompositiob of argv[1]
    char ar[100];
    strcpy(ar,path1);
    char ** tokens = decompose(ar,"/");

    //Will be the name of the tar to open
    char tar[100];

    //Will be the name of the copy
    char path[100];

    //Reset tar to "" in case there is a issue
    strcpy(tar,"");

    //While we dont see a the name of the file strcat the path to the tar
    while(string_contains_tar(tokens[i2]) != 1){
      strcat(tar,tokens[i2]);
      strcat(tar,"/");
      i2++;
    }
    
    //Final strcat to cpy the name of the file
    strcat(tar,tokens[i2]); 
    i2++;

    //Reset path by the the first argument after the name of the tar
    strcpy(path,tokens[i2]);
    i2++;

    //While there are still argument, copy the path 
    while(tokens[i2] != NULL){
      strcat(path,"/");
      strcat(path,tokens[i2]);    
      i2++;
    }

    // OPENING THE TAR FILE
    int fd=open(tar,O_RDWR);

    printf("\n%s\n",tar);

    create_dir(fd,argv[i],path);
    close(fd);

	}

	return 0;
}
