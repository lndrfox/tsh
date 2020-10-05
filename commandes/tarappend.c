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



int main(int argc, char *argv[]){

 
  int n;
  char tmp[512];
  char buf [BLOCKSIZE];
  struct posix_header hd;
  unsigned int size;

   if (argc < 4 || argc > 4) {
    printf("Need 3 argument of type -tarappend name.tar file_copy name_of_the_copy-");
    return -1;
  }

  //TAR A OUVRIR
  int fd = open(argv[1], O_RDWR);

  if(fd <0){
    perror("fuck argv 1");
    exit(-1);
  }

  //FICHIER A COPIER
  int fd2 = open(argv[2], O_RDONLY);

  if(fd2<0){
    perror("fuck argv 2");
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

    //IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE HEADER THEN IT DOESNT EXIST AND WE CAN CREATE IT

    if((hd.name[1]=='\0')){

      break;
    }

		  			
    //READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

    unsigned int size;
    sscanf(hd.size, "%o",&size);

    //WE GET TO THE NEXT HEADER

    lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(hd.name[1]!=0);

  
  struct posix_header temporaire;

  
//INIT STRUCT MEMORY

 memset(&temporaire,0,BLOCKSIZE);

 //  WE NEED TO ADD / AT THE END OF THE DIRECTORY NAME FOR IT TO BE VALID

 sprintf(temporaire.name,"%s",argv[3]);

  
  //FILLING MODE 

 sprintf(temporaire.mode,"0000700");

 //SIZE IS FILE OF COPIED FILE

  unsigned int fsize;
  fsize = lseek(fd2,0,SEEK_END);
  printf("copy size = %d", fsize);
  sprintf(temporaire.size,"%011o",fsize);
  lseek(fd2,0,SEEK_SET);


  //FILLING MAGIC FIELD


 sprintf(temporaire.magic,TMAGIC);


 //FILE SO TYPE IS 0

 temporaire.typeflag='0';

 // VERSION

 sprintf(temporaire.version,TVERSION);

 //USER NAME

 getlogin_r(temporaire.uname, sizeof(temporaire.uname));

 //GETTING GROUP NAME

 gid_t gid= getgid();
 struct group * g= getgrgid(gid);

 //ERROR MANAGMENT

 if(g==NULL){
   perror("Reading group ID");
   exit(-1);
 }
 sprintf(temporaire.gname,"%s",g->gr_name);

 //SETTING CHECKSUM ONCE ALL THE OTHER FIELDS ARE FILLED
 
 set_checksum(&temporaire);
 check_checksum(&temporaire);
		
 // SINCE WE READ THE FIRST EMPTY BLOCK (TAR ENDS WITH 2 EMPTY BLOCKS) TO CHECK IF WE HAD READ ALL OF THE HEADERS
 // WE NEED TO GO BACK ONE BLOCK



 //WRITING THE NEW DIRECTORY AT THE END OF THE FILE
 
 lseek(fd,-512,SEEK_CUR);
  int rddd=write(fd,&temporaire,BLOCKSIZE);

  if(rddd<BLOCKSIZE){

			perror("Error writing in file");
			exit(-1);
		}

  
  char buff [BLOCKSIZE];

  for(unsigned int i=0; i<((fsize+ BLOCKSIZE - 1) >> BLOCKBITS); i++){

    
    printf("fuck la boucle\n");
    
			int rdtmp = read(fd2,buff, BLOCKSIZE);
			printf("%s",buff);
			printf("%d\n", rdtmp);
			//EROR MANAGMENT

			if(rdtmp<0){

				perror("Reading tar file");
				exit(-1);
			}

			//WRITING THE BLOCK AND ERROR MANGEMENT

			if(write(fd,buff, BLOCKSIZE)<0){

				perror("Writing file content");
				exit(-1);

			}

			//RESETING THE BUFFER

				memset(buff, 0, BLOCKSIZE);
  }

    
 
  memset(buf,0,BLOCKSIZE);

  printf("allo");
  
   for(int i=0;i<2;i++){

     printf("et la so coman");
     int rdd=write(fd,buf,BLOCKSIZE);


     if(rdd<BLOCKSIZE){

       perror("Error writing in file2");
       exit(-1);
     }

   }


  
  lseek(fd,0,SEEK_SET);    
  close (fd);
  close (fd2);
  return 0;
}
 
   

