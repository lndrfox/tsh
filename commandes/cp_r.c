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
#include <dirent.h>
#include <libgen.h>
#include <limits.h>

//mkdirep est le même code que mkdir sauf qu'elle a était modifié pour qu'on puisse l'utiliser
//de façon récursive
int mkdirep(char *argv){

	char * tar=getenv("tar");

	// OPENING THE TAR FILE

	int fd=open(tar,O_RDWR);

	//ERROR MANAGMENT

	if(fd==-1){
		perror("open tar file");
		exit(-1);
	}

	// THIS IS WERE WE STORE HEADERS

	struct posix_header hd;

		// WE LOOP ON EVERY DIRECTORY THAT WE NEED TO CREATE

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

			if((hd.name[0]=='\0')){

				break;
			}

			//IF WE FOUND THE HEADER, IT ALREADY EXISTS AND WE HAVE AN ERROR

			if(strcmp(hd.name,argv)==0){

				prints("Error, the directory ");
				prints(argv);
				prints(" already exists");
				return -1;

			}

			
			//READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			unsigned int size;
			sscanf(hd.size, "%o",&size);

			//WE GET TO THE NEXT HEADER

			lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


		}while(strcmp(hd.name,argv)!=0);

		//CREATING THE HEADER FOR THE NEW DIRECTORY


		struct posix_header dir;

		//INIT STRUCT MEMORY

		memset(&dir,0,BLOCKSIZE);

		//  WE NEED TO ADD / AT THE END OF THE DIRECTORY NAME FOR IT TO BE VALID

		char name_tmp[100];
		sprintf(name_tmp, "%s", argv);
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




		//WE GO BACK TO THE BEGINNING OF THE TAR FILE TO MAKE SURE THAT WE DONT CREATE A DIRECTORY THAT ALREADY EXISTS 
		// FOR THE NEXT ARGUMENT

		lseek(fd,0,SEEK_SET);


	
	

	close(fd);

	

	return 0;
}

//ext_vers_tar2 est le même code que ext_vers_tar sauf qu'elle a était modifié pour qu'on puisse l'utiliser
//de façon récursive

//COPY A FILE FROM THE CURRENT DIRECTORY  INTO THE TAR
//the format is -- cp  path/of/file name_of_the_tar.tar name_of_copy
int ext_vers_tar(char *argv[]){
  
  char buf [BLOCKSIZE];
  struct posix_header hd;//Header for the tar

 char * tar=get_tar_name("tar");


	// OPENING THE TAR FILE

	int fd=open(tar,O_RDWR);

	
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

  sprintf(temporaire.name,"%s",argv[2]);

  
  //FILLING MODE 

 sprintf(temporaire.mode,"0000700");

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

  for(unsigned int i=0; i<((fsize+ BLOCKSIZE - 1) >> BLOCKBITS); i++){
 
    int rdtmp = read(fd2,buff, BLOCKSIZE);
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
//the format is -- cp nameoftar.tar path/of/file nameof2tar.tar name_of_copy
int tar_vers_tar(char *argv[]){

  //HEADER FOR THE FIRST TAR WE COPY THE FILE
  struct posix_header hd;

  //HEADER FOR THE SECOND TAR WE COPIED FILE WILL BE
  struct posix_header hd2;

  //Size of the file
  unsigned int size;
  unsigned int size2;

  char *ar1;
  ar1 = argv[1];
  char *ar2;
  ar2 = argv[2];
 
 

  char ** tokens = decompose((ar1),"/");
  char ** tokens2 = decompose((ar2),"/");
  char * tar;
  char * tar2;

  int fd;
  int fd3;
  if(string_contains_tar(tokens[0])){
    tar = tokens[0];
  }
  else{
  //FIRST TAR THE FILE IS IN
    tar=getenv("tar");
    // Si la variable d'nevironnement est dans un repertoire du tar
    char * var_rep = NULL;
    if(strchr(tar, '/') != NULL) {
      strtok(tar, "/");
      char * var_r = strtok(NULL, "");
      var_rep = malloc(strlen(var_r) + 2);
      strcpy(var_rep, var_r);
      strcat(var_rep, "/");
    }

  }


  // OPENING THE TAR FILE
  fd=open(tar,O_RDWR);

	
  if(string_contains_tar(tokens2[0])){
    tar2 = tokens2[0];
    fd3 = open(tar2,O_RDWR);
    
  }
  else{
    //SECOND TAR THE FILE WILL BE COPIED
    fd3 = open(argv[2],O_RDWR);
  }
 
  
  char rd [BLOCKSIZE] ;

  //OPENING FIRST TAR AND FINDING FILE
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

    //READONG THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			
    sscanf(hd.size, "%o",&size);

    //IF WE FOUND THE RIGHT HEADER, WE GET OUT OF THE LOOP

    if(strcmp(getenv("tar"),tar)==0){
	if(strcmp(hd.name,argv[1])==0){
 		break;
	}

    }

    //OTHERWISE WE GET TO THE NEXT HEADER

    lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(strcmp(hd.name,argv[1])!=0);



  ////////////////////////////////////////////
  ////////////////////////////////////////////
  //OPENING SECOND TAR GO TO THE END

   do{
    // READING AN HEADER

    int rdcount=read(fd3,&hd2,BLOCKSIZE);

    //ERROR MANAGMENT

    if(rdcount<0){
      prints("error e");
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

    lseek(fd3,((size2+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(hd2.name[1]!=0);

   //Header we will put at the end of the second tar
  struct posix_header temporaire;

  
//INIT STRUCT MEMORY

 memset(&temporaire,0,BLOCKSIZE);

 // CHOOSE NAME FILE
 if((tokens2[1])!=NULL){
     sprintf(temporaire.name,"%s",tokens2[1]);
   }
 else{
   sprintf(temporaire.name,"%s",argv[2]);
 }

  
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

  for(unsigned int i=0; i<(size+ BLOCKSIZE - 1);i++){
		  
    int rdtmp = read(fd, buff, 1);

    //EROR MANAGMENT
    if((strcmp(buff,"\0"))==0){
      prints("End");
      break;
    }
    
    if(rdtmp<0){
      prints("error z");
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



//plus ou moins le même code que cp_r sauf quel a été modifié pour que l'on puisse
//l'utiliser de façon récursive
int cp_r_aux(char *argv[]){
  DIR *dirp = opendir(argv[1]);
  struct dirent *entry;
  prints("deb");
   prints("\n");
  char temp [100];
  char temp2[100];
  char *arg [3];
  arg[1] = temp;
  arg[2] = temp2;
  strcpy(temp2,argv[2]);
 
  prints(temp2);
  while((entry=readdir(dirp))){
    if((strcmp((entry->d_name),".") != 0) && ( strcmp((entry->d_name),"..") !=0) ){
       strcpy(temp,argv[1]);
       strcat(temp,"/");
       strcat(temp,entry->d_name);
      prints(entry->d_name);
    
       if(entry->d_type== DT_REG){	
	 //strcat(temp,"/");
	 strcpy(temp2,argv[2]);
	 strcat(temp2,"/");
	 strcat(temp2,entry->d_name);
	 prints("fic");
	 prints("\n");
	 prints(temp2);
	ext_vers_tar(arg);
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
      }
      if(entry->d_type== DT_DIR){
	//strcat(temp,"/");
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
	strcat(temp2,entry->d_name);
	prints("rep");
	 prints("\n");
	mkdirep(temp2);
	cp_r_aux(arg);
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
      }
     
    }
    
  }
  return 0;
}

int cp_r(char *argv[]){

  DIR *dirp = opendir(argv[1]);
  struct dirent *entry;
  prints("deb");
   prints("\n");
  char temp [100];
  char temp2[100];
  char *arg [3];
  arg[1] = temp;
  arg[2] = temp2;
  strcpy(temp2,argv[2]);
 
  prints(temp2);
  mkdirep(temp2);
  while((entry=readdir(dirp))){
    if((strcmp((entry->d_name),".") != 0) && ( strcmp((entry->d_name),"..") !=0) ){
       strcpy(temp,argv[1]);
       strcat(temp,"/");
       strcat(temp,entry->d_name);
      prints(entry->d_name);
    
       if(entry->d_type== DT_REG){	
	 //strcat(temp,"/");
	 strcpy(temp2,argv[2]);
	 strcat(temp2,"/");
	 strcat(temp2,entry->d_name);
	 prints("fic");
	 prints("\n");
	 prints(temp2);
	ext_vers_tar(arg);
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
      }
      if(entry->d_type== DT_DIR){
	//strcat(temp,"/");
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
	strcat(temp2,entry->d_name);
	prints("rep");
	 prints("\n");
	mkdirep(temp2);
	cp_r_aux(arg);
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
      }
     
    }
    
  }
  return 0;
}

//plus ou moins le même code que cp_r sauf quel a été modifié pour que l'on puisse
//l'utiliser de façon récursive
int cp_r_aux_tvt(char *argv[]){
  DIR *dirp = opendir(argv[1]);
  struct dirent *entry;
  prints("deb");
   prints("\n");
  char temp [100];
  char temp2[100];
  char *arg [3];
  arg[1] = temp;
  arg[2] = temp2;
  strcpy(temp2,argv[2]);
 
  prints(temp2);
  while((entry=readdir(dirp))){
    if((strcmp((entry->d_name),".") != 0) && ( strcmp((entry->d_name),"..") !=0) ){
       strcpy(temp,argv[1]);
       strcat(temp,"/");
       strcat(temp,entry->d_name);
      prints(entry->d_name);
    
       if(entry->d_type== DT_REG){	
	 //strcat(temp,"/");
	 strcpy(temp2,argv[2]);
	 strcat(temp2,"/");
	 strcat(temp2,entry->d_name);
	 prints("fic");
	 prints("\n");
	 prints(temp2);
	tar_vers_tar(arg);
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
      }
      if(entry->d_type== DT_DIR){
	//strcat(temp,"/");
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
	strcat(temp2,entry->d_name);
	prints("rep");
	 prints("\n");
	mkdirep(temp2);
	cp_r_aux_tvt(arg);
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
      }
     
    }
    
  }
  return 0;
}

int cp_r_tvt(char *argv[]){
  DIR *dirp = opendir(argv[1]);
  struct dirent *entry;
  prints("deb");
   prints("\n");
  char temp [100];
  char temp2[100];
  char *arg [3];
  arg[1] = temp;
  arg[2] = temp2;
  strcpy(temp2,argv[2]);
 
  prints(temp2);
  mkdirep(temp2);
  while((entry=readdir(dirp))){
    if((strcmp((entry->d_name),".") != 0) && ( strcmp((entry->d_name),"..") !=0) ){
       strcpy(temp,argv[1]);
       strcat(temp,"/");
       strcat(temp,entry->d_name);
      prints(entry->d_name);
    
       if(entry->d_type== DT_REG){	
	 //strcat(temp,"/");
	 strcpy(temp2,argv[2]);
	 strcat(temp2,"/");
	 strcat(temp2,entry->d_name);
	 prints("fic");
	 prints("\n");
	 prints(temp2);
	tar_vers_tar(arg);
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
      }
      if(entry->d_type== DT_DIR){
	//strcat(temp,"/");
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
	strcat(temp2,entry->d_name);
	prints("rep");
	 prints("\n");
	mkdirep(temp2);
	cp_r_aux_tvt(arg);
	strcpy(temp2,argv[2]);
	strcat(temp2,"/");
      }
     
    }
    
  }
  return 0;
}

int main(int argc,char *argv[]){
  // cp_r(argv);
   cp_r_tvt(argv);
  return argc;
}
