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

//COPY A FILE FROM A TAR TO A REP OUTSIDE THE TAR
int tar_vers_ext(char *argv[]){

  //Will be a counter 
  int i = 0;

  struct posix_header hd; //Header for the tar

  unsigned int size; //Size of the file that will be initialized later

  //Array of the decompositiob of argv[1]
  char ar[100];
  strcpy(ar,argv[1]);
  char ** tokens = decompose(ar,"/");

  //Will be the name of the tar to open
  char tar[100];

  //Will be the name of the copy
  char path[100];

  //Reset tar to "" in case there is a issue
  strcpy(tar,"");

  //While we dont see a the name of the file strcat the path to the tar
  while(string_contains_tar(tokens[i]) != 1){
    strcat(tar,tokens[i]);
    strcat(tar,"/");
    i++;
  }
  //Final strcat to cpy the name of the file
  strcat(tar,tokens[i]); 

  i++;

  //Reset path by the the first argument after the name of the tar
  strcpy(path,tokens[i]);
  i++;

  //While there are still argument, copy the path 
  while(tokens[i] != NULL){
    strcat(path,"/");
    strcat(path,tokens[i]);    
    i++;
  }

  // OPENING THE TAR FILE

  int fd=open(tar,O_RDWR);
  //ERROR MANAGMENT

  if(fd==-1){
    perror("open tar file");
    exit(-1);
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
      printsss("cp: impossible d'évaluer '", path ,"' : Aucun fichier ou dossier de ce type\n");  
      return -1;
    }

    //READONG THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			
    sscanf(hd.size, "%o",&size);

    //IF WE FOUND THE RIGHT HEADER, WE GET OUT OF THE LOOP

    if(strcmp(hd.name,path)==0){
      break;
    }

    

    //OTHERWISE WE GET TO THE NEXT HEADER

    lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(strcmp(hd.name,path)!=0);

  //CREATING THE FILE TO COPY

  int fd2=open(argv[2], O_WRONLY | O_CREAT , S_IRUSR | S_IWUSR);       
  
  //GETTING THE SIZE OF WHAT WE NEED TO READ

  char rd [BLOCKSIZE] ;

  //LOOP TO READ AND WRITE THE BLOCKS OF THE FILE CONTENT

    for(unsigned int i=0; i<(size);i++){
      
      
    int rdtmp = read(fd, rd, 1);
   
    //EROR MANAGMENT
    
   
    if((strcmp(rd,"\0"))==0){
     
      break;
    }
    if(rdtmp<0){

      perror("Reading tar file");
      exit(-1);
    }

    //WRITING THE BLOCK AND ERROR MANGEMENT
   
    if(write(fd2,rd, 1)<0){

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
  
return 0;
}


//COPY A FILE FROM OUTSIDE OF THE TAR  INTO A TAR
int ext_vers_tar(char *argv[]){
 
  char buf [BLOCKSIZE];

  //Will be a counter 
  int i = 0;

  struct posix_header hd; //Header for the tar

  unsigned int size; //Size of the file that will be initialized later

  //Array of the decompositiob of argv[2]
  char ar[100];
  strcpy(ar,argv[2]);
  char ** tokens = decompose(ar,"/");

  //Will be the name of the tar to open
  char tar[100];

  //Will be the name of the copy
  char path[100];

  //Reset tar to "" in case there is a issue
  strcpy(tar,"");

  //While we dont see a the name of the file strcat the path to the tar
  while(string_contains_tar(tokens[i]) != 1){
    strcat(tar,tokens[i]);
    strcat(tar,"/");
    i++;
  }
  //Final strcat to cpy the name of the file
  strcat(tar,tokens[i]); 

  i++;

  //Reset path by the the first argument after the name of the tar
  strcpy(path,tokens[i]);
  i++;

  //While there are still argument, copy the path 
  while(tokens[i] != NULL){
    strcat(path,"/");
    strcat(path,tokens[i]);    
    i++;
  }


  // OPENING THE TAR FILE

  int fd=open(tar,O_RDWR);

	
  if(fd <0){
    perror("Problem with argv 2");
    exit(-1);
  }
  
  int fd2 = open(argv[1], O_RDONLY);

  if(fd2<0){
    printsss("cp: impossible d'évaluer '", path ,"' : Aucun fichier ou dossier de ce type\n");  
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

    sscanf(hd.size, "%o",&size);

    //WE GET TO THE NEXT HEADER

    lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);

    
  }while(hd.name[1]!=0);//While the header is not at the end block of 0

  
  struct posix_header temporaire;//The 'entete' we will put at the end of the tar

  
  //INIT STRUCT MEMORY

  memset(&temporaire,0,BLOCKSIZE);

  //Naming the file as argv[3]

  sprintf(temporaire.name,"%s",path);

  
  //FILLING MODE 

 sprintf(temporaire.mode,"0000777");

 //SIZE BECOME THE SIZE OF THE COPIED FILE

  unsigned int fsize;//Size of the file that will be initialized later
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

  for(unsigned int i=0; i<(fsize); i++){
 
    int rdtmp = read(fd2,buff, 1);
    //EROR MANAGMENT

    if(rdtmp<0){
      perror("Reading tar file");
      exit(-1);
    }

    //WRITING THE BLOCK AND ERROR MANGEMENT

    if(write(fd,buff, 1)<0){

      perror("Writing file content");
      exit(-1);

    }

    //RESETING THE BUFFER

    memset(buff, 0, BLOCKSIZE);
  }

    
 
  memset(buf,0,BLOCKSIZE);
  
  for(int i=0;i<2;i++){

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

//COPY A FILE FROM A TAR INTO ANOTHER TAR
int tar_vers_tar(char *argv[]){
 
  //HEADER FOR THE FIRST TAR WE COPY THE FILE
  struct posix_header hd;

  //HEADER FOR THE SECOND TAR WE COPIED FILE WILL BE
  struct posix_header hd2;

  //Size of the file
  unsigned int size;
  unsigned int size2;

  //Copy argv1 into ar1
  char ar1[100];
  strcpy(ar1, argv[1]);

  //Copy argv2 into ar2
  char ar2 [100];
  strcpy(ar2, argv[2]);
 
  //Decomposing ar1 and ar2
  char ** tokens = decompose((ar1),"/");
  char ** tokens2 = decompose((ar2),"/");
  
  //To be use to browse the first tar
  int fd;

  //To be use to browse the second tar
  int fd2;


  //Will be the path to the first tar
  char tar[100];

  //Will be the path to the second tar
  char tar2[100];

  //Will be the path to where the first copy will be
  char path[100];

  //Will be the path to where we copy the file in
  char path2[100];

  //Will be counter
  int i=0;
  int i2=0;

  //Reseting tar in case there was something already
  strcpy(tar,"");

  //While the name of the path is not found copy the path to the tar
  while(string_contains_tar(tokens[i]) != 1){
    strcat(tar,tokens[i]);
    strcat(tar,"/");
    i++;
  }

  //Add the name of the tar to the end
  strcat(tar,tokens[i]); 

  i++;
  //Reseting path to tokens[i] wich is the first argument after the name of the tar
  strcpy(path,tokens[i]);
  i++;

  //While the path a the file to copy from is not found we copy the path to the file
  while(tokens[i] != NULL){
    strcat(path,"/");
    strcat(path,tokens[i]);    
    i++;
  }

  //Open the first tar
  fd=open(tar,O_RDWR);

  //Everything is the same as above
  //But its to where we copy the file in
  strcpy(tar2,"");
 
  while(string_contains_tar(tokens2[i2]) != 1){
   
    strcat(tar2,tokens2[i2]);
    strcat(tar2,"/");
    i2++;
  }

  strcat(tar2,tokens2[i2]); 

  i2++;
  strcpy(path2,tokens2[i2]);
  i2++;
  
  while(tokens2[i2] != NULL){
    strcat(path2,"/");
    strcat(path2,tokens2[i2]);    
    i2++;
  }

  // OPENING THE TAR FILE WHERE THE FILE IS
  fd2=open(tar2,O_RDWR);
 
  
  char rd [BLOCKSIZE] ;
  //OPENING FIRST TAR AND FINDING FILE
  do{
   
    // READING AN HEADER

    int rdcount=read(fd,&hd,BLOCKSIZE);
  
    //ERROR MANAGMENT

    if(rdcount<0){
      perror("reading tar file\n");
      close(fd);
      return -1;
    }

    //IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE GOOD HEADER
  
    if((hd.name[0]=='\0')){   
      printsss("cp: impossible d'évaluer '", argv[1] ,"' : Aucun fichier ou dossier de ce type \n");  
      return -1;
    }

    //READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			
    sscanf(hd.size, "%o",&size);

    //IF WE FOUND THE RIGHT HEADER, WE GET OUT OF THE LOOP
    
    if(strcmp(hd.name,path) == 0){	
      break;
    }   
    
      

    //OTHERWISE WE GET TO THE NEXT HEADER
     
    lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);
   

  }while(strcmp(hd.name,path)!=0);


  ////////////////////////////////////////////
  ////////////////////////////////////////////
  //OPENING SECOND TAR GO TO THE END

   do{
    
    // READING AN HEADER

    int rdcount=read(fd2,&hd2,BLOCKSIZE);
  
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

    sscanf(hd2.size, "%o",&size2);

    //WE GET TO THE NEXT HEADER

    lseek(fd2,((size2+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(hd2.name[1]!=0);
   
   //Header we will put at the end of the second tar
  struct posix_header temporaire;

  
//INIT STRUCT MEMORY

 memset(&temporaire,0,BLOCKSIZE);

 // CHOOSE NAME FILE
 sprintf(temporaire.name,"%s",path2);
   
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

  lseek(fd2,-512,SEEK_CUR);

  
 //WRITING THE NEW DIRECTORY AT THE END OF THE FILE
 

  int rddd=write(fd2,&temporaire,BLOCKSIZE);


  
  if(rddd<BLOCKSIZE){

    perror("Error writing in file");
    exit(-1);
  }

  
  char buff [BLOCKSIZE];

  memset(buff,0,BLOCKSIZE);

  for(unsigned int i=0; i<((size+ BLOCKSIZE - 1) >> BLOCKBITS);i++){
		  
    int rdtmp = read(fd, buff, BLOCKSIZE);
   
    //EROR MANAGMENT
    if((strcmp(buff,"\0"))==0){
      break;
    }
    
    if(rdtmp<0){
      perror("Reading tar file");
      exit(-1);
    }

    //WRITING THE BLOCK AND ERROR MANGEMENT

    if(write(fd2,buff, BLOCKSIZE)<0){

      perror("Writing file content");
      exit(-1);

    }

       
    memset(rd, 0, BLOCKSIZE);			

  }

  
  memset(buff,0,BLOCKSIZE);
   for(int i=0;i<2;i++){

    int rdd=write(fd2,buff,BLOCKSIZE);


    if(rdd<BLOCKSIZE){

      perror("Error writing in file2");
      exit(-1);
    }

   }
  lseek(fd2,0,SEEK_SET);
  lseek(fd,0,SEEK_SET); 
  close (fd);
  close (fd2);
  return 0;
}


int main (int argc, char *argv[]){

  //Get a variable containing argv[1]
  char *test;
  test=argv[1];

  //Get a variable containing argv[2]
  char *test2;
  test2=argv[2];

  //We set a path removing every .. for argv1
  char path1[100]; 
  strcpy(path1,true_path(test));

  //We set a path removing every .. for argv2
  char path2[100];
  strcpy(path2,true_path(test2));

  //Reseting argv1 and argv2 by their true_path
  argv[1] = path1;
  argv[2] = path2;

  if (argc == 3){

    //if argv1 is not inside a tar and argv2 is insiede a tar call ext_vers_tar
    if((string_contains_tar(argv[1]) == 0) && (string_contains_tar(argv[2]) == 1)){
      if (ext_vers_tar(argv) == 0){	
	return 0;     
      }
    }
    
    //if argv1 is inside a tar and argv2 is not inside a tar call _tar_vers_ext
    if((string_contains_tar(argv[1]) == 1) && (string_contains_tar(argv[2]) == 0)){    
      if(tar_vers_ext(argv)){
	return 0;
      }
    }

    //if argv1 and argv2 are inside tar then call tar_vers_tar
    if((string_contains_tar(argv[1]) == 1) && (string_contains_tar(argv[2]) == 1)){ 
      if (tar_vers_tar(argv)==0){
	return 0;
      }  
    }
  }
  else{
    prints("cp needs 2 argument of the format cp copied_file paste_file \n");
    return -1;
  }
  return 0;
}
