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

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
int tar_vers_ext_dir (int argc, char *argv[]){
  return 0;
}


//COPY A FILE FROM A TAR INTO THE CURRENT REPERTORY
//the format is -- cp nameoftar.tar path/of/file name_of_copy
int tar_vers_ext(int argc, char *argv[]){
  
  if (argc < 4 || argc > 4) {
    printf("Need 3 argument of type -cp name.tar pathname/of/file name_of_copy-");
    return -1;
  }
  
  struct posix_header hd; //Header for the tar

  unsigned int size; //Size of the file that will be initialized later
  
  int fd = open(argv[1], O_RDONLY); // Argv[1] is the tar we open
  
  if (fd < 0) return -1;
  
  if(fd == -1){
    printf("argument 1 need to be a .tar\n");
    return -1;
  }
  

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

      printf("Error, file not found int the .tar\n");
      return -1;
    }

    //READONG THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			
    sscanf(hd.size, "%o",&size);

    //IF WE FOUND THE RIGHT HEADER, WE GET OUT OF THE LOOP

    if(strcmp(hd.name,argv[2])==0){

      break;

    }

    //OTHERWISE WE GET TO THE NEXT HEADER

    lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(strcmp(hd.name,argv[2])!=0);

  //CREATING THE FILE TO COPY

  else{
  int fd2=open(argv[3], O_WRONLY | O_CREAT , S_IRUSR | S_IWUSR);

  //GETTING THE SIZE OF WHAT WE NEED TO READ

  char rd [BLOCKSIZE] ;

  //LOOP TO READ AND WRITE THE BLOCKS OF THE FILE CONTENT

  for(unsigned int i=0; i<(size+ BLOCKSIZE - 1) >> BLOCKBITS;i++){
		  
    int rdtmp = read(fd, rd, BLOCKSIZE);

    //EROR MANAGMENT

    if(rdtmp<0){

      perror("Reading tar file");
      exit(-1);
    }

    //WRITING THE BLOCK AND ERROR MANGEMENT

    if(write(fd2,rd, rdtmp)<0){

      perror("Writing file content");
      exit(-1);

    }
      
    memset(rd, 0, BLOCKSIZE);			
  }
  
  //MOVING READING HEAD BACK TO THE BEGINING OF THE TAR FILE IN CASE ARGUMENTS ARE NOT
  //IN THE SAME ORDER AS THE HEADERS IN THE TAR FILE

  lseek(fd,0,SEEK_SET);

  //CLOSING WRITING FILE
		
  close(fd);
  close(fd2);
  }


return 0;
}


//COPY A FILE FROM THE CURRENT DIRECTORY  INTO THE TAR
//the format is -- cp  path/of/file name_of_the_tar.tar name_of_copy
int ext_vers_tar(int argc, char *argv[]){
  
 
  int n;
  char tmp[512];
  char buf [BLOCKSIZE];
  struct posix_header hd;//Header for the tar
  unsigned int size;//Size of the file that will be initialized later

   if (argc < 4 || argc > 4) {
    printf("Need 3 argument of type -tarappend name.tar file_copy name_of_the_copy-");
    return -1;
  }

  //TAR TO OPEN
  int fd = open(argv[2], O_RDWR);

  if(fd <0){
    perror("Problem with argv 2");
    exit(-1);
  }

  //FILE TO COPY
  int fd2 = open(argv[1], O_RDONLY);

  if(fd2<0){
    perror("Poblem with argv 1");
    exit(-1);
  }

  
  // THIS LOOP ALLOWS US TO GO TO THE END OF THE TAR
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

    
  }while(hd.name[1]!=0);//While the header is not at the end block of 0

  
  struct posix_header temporaire;//The 'entete' we will put at the end of the tar

  
  //INIT STRUCT MEMORY

  memset(&temporaire,0,BLOCKSIZE);

  //Naming the file as argv[3]

  sprintf(temporaire.name,"%s",argv[3]);

  
  //FILLING MODE 

 sprintf(temporaire.mode,"0000700");

 //SIZE BECOME THE SIZE OF THE COPIED FILE

  unsigned int fsize;
  fsize = lseek(fd2,0,SEEK_END);//GET FILE SIZE


  //SETTING THE ENTETE SIZE AS THE SAME OF THE COPIED FILE
  
  sprintf(temporaire.size,"%011o",fsize);
  lseek(fd2,0,SEEK_SET);//RESETING fd2 OFFSET


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

 lseek(fd,-512,SEEK_CUR);

 //WRITING THE NEW DIRECTORY AT THE END OF THE FILE
 

  int rddd=write(fd,&temporaire,BLOCKSIZE);

  if(rddd<BLOCKSIZE){

			perror("Error writing in file");
			exit(-1);
		}

  
  char buff [BLOCKSIZE];

  for(unsigned int i=0; i<((fsize+ BLOCKSIZE - 1) >> BLOCKBITS); i++){
 
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


int tar_vers_tar(int argc, char *argv[]){
  int n;
  char tmp[512];

  //HEADER FOR THE FIRST TAR WE COPY THE FILE
  struct posix_header hd;

  //HEADER FOR THE SECOND TAR WE COIED FILE WILL BE
  struct posix_header hd2;

  //Size of the file
  unsigned int size;
  unsigned int size2;

  //FIRST TAR THE FILE IS IN
  int fd = open(argv[1], O_RDONLY);

  //SECOND TAR THE FILE WILL BE COPIED
  int fd3 = open(argv[3],O_RDWR);
  
  char rd [BLOCKSIZE] ;

  //OPENING FIRST TAR AND FINDING FILE
  do{
    printf("1\n");
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

      printf("Error, file not found int the .tar\n");
      return -1;
    }

    //READONG THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			
    sscanf(hd.size, "%o",&size);

    //IF WE FOUND THE RIGHT HEADER, WE GET OUT OF THE LOOP

    if(strcmp(hd.name,argv[2])==0){

      break;

    }

    //OTHERWISE WE GET TO THE NEXT HEADER

    lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(strcmp(hd.name,argv[2])!=0);



  ////////////////////////////////////////////
  ////////////////////////////////////////////
  //OPENING SECOND TAR GO TO THE END

   do{
     printf("2\n");
    // READING AN HEADER

    int rdcount=read(fd3,&hd2,BLOCKSIZE);

    //ERROR MANAGMENT

    if(rdcount<0){

      perror("reading tar file");
      close(fd);
      return -1;
    }

    //IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE HEADER THEN IT DOESNT EXIST AND WE CAN CREATE IT

    if((hd2.name[1]=='\0')){

      break;
    }

		  			
    //READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

    unsigned int size;
    sscanf(hd2.size, "%o",&size2);

    //WE GET TO THE NEXT HEADER

    lseek(fd3,((size2+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(hd2.name[1]!=0);

   //Header we will put at the end of the second tar
  struct posix_header temporaire;

  
//INIT STRUCT MEMORY

 memset(&temporaire,0,BLOCKSIZE);

 // CHOOSE NAME FILE

 sprintf(temporaire.name,"%s",argv[4]);

  
  //FILLING MODE 

 sprintf(temporaire.mode,"0000700");

 //SIZE IS FILE OF COPIED FILE

 unsigned int temp_size;
 sscanf(hd.size,"%o",&temp_size);
 sprintf(temporaire.size,"%011o",temp_size);

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

  lseek(fd3,-512,SEEK_CUR);

  
 //WRITING THE NEW DIRECTORY AT THE END OF THE FILE
 

  int rddd=write(fd3,&temporaire,BLOCKSIZE);


  
  if(rddd<BLOCKSIZE){

			perror("Error writing in file");
			exit(-1);
		}

  
  char buff [BLOCKSIZE];

  memset(buff,0,BLOCKSIZE);

  for(unsigned int i=0; i<(size+ BLOCKSIZE - 1) >> BLOCKBITS;i++){
		  
    int rdtmp = read(fd, buff, BLOCKSIZE);

    //EROR MANAGMENT

    if(rdtmp<0){

      perror("Reading tar file");
      exit(-1);
    }

    //WRITING THE BLOCK AND ERROR MANGEMENT

    if(write(fd3,buff, rdtmp)<0){

      perror("Writing file content");
      exit(-1);

    }

       
    memset(rd, 0, BLOCKSIZE);			

  }

  
  memset(buff,0,BLOCKSIZE);
  for(int i=0;i<2;i++){
    printf("fuck3");

    int rdd=write(fd3,buff,BLOCKSIZE);


    if(rdd<BLOCKSIZE){

      perror("Error writing in file2");
      exit(-1);
    }

  }
  lseek(fd3,0,SEEK_SET);
  lseek(fd,0,SEEK_SET); 
  close (fd);
  close (fd3);
  return 0;
}




int main (int argc, char *argv[]){

 
  if(argc == 4 || argc == 5){
    if(argc == 4){
      if(strchr(argv[1],'.') != NULL){
	 printf("tar vers ext\n");
	    tar_vers_ext(argc,argv);
	    return 0;
      }
      else {
	  printf("ext vers tar\n");
	  ext_vers_tar(argc,argv);
	  return 0; 	   
      }
      if (argc == 5){
	printf("tar vers tar\n");
	tar_vers_tar(argc,argv);
      }
      
    }
    else {return -1;}
  }

  return 0;
}
