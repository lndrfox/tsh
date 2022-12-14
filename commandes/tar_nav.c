#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <grp.h>
#include <time.h>

#include "tar.h"
#include "print.h"
#include "tar_nav.h"
#include "lib.h"



/*DECOMPOSE THE STRING PROMPT ACCORDING TO THE STRING DELIMITER AND RETURNS THE TOKENS
IN CHAR ** TOKEN ENDING BY NULL*/

char ** decompose(char * prompt, char * delimiter){

	/*WE CALCULATE HOW MANY TOKENS THE STRING HAS*/
	char * prompt_cpy=(char * ) malloc(strlen(prompt)+sizeof(char));
	strcpy(prompt_cpy,prompt);

	char * check_len = strtok(prompt_cpy, delimiter);

	int i =1;

	while((check_len =strtok(NULL, delimiter) ) !=NULL){

		i++;

	}

	free(check_len);
	free(prompt_cpy);

	/* THIS WERE WE STORE ALL THE TOKENS*/
	char ** tokens= (char **) calloc(i+1,sizeof(char *));

	/*ERROR MANAGEMENT*/

	if(tokens ==NULL){

		exit (-1);
	}

	/*WE NEED TO STORE THE FIRST RESULT OF STRTOK TO NOT LOSE ANYTHING*/
	char * token = strtok(prompt, delimiter);
	int cpt_tokens=0;
	tokens[cpt_tokens]=token;
	cpt_tokens++;

	/*AS LONG AS THERE IS SOMETHING TO DECOMPOSE WE TOK AND STORE IT IN TOKENS*/

	while((token =strtok(NULL, delimiter) ) !=NULL){

		tokens[cpt_tokens]= token;
		cpt_tokens++;

	}

	/*WE ADD NULL AT THE END OF TOKENS */

	tokens[cpt_tokens]=token;

	free(token);
	return tokens;

}


/*CHECKS IF THE STRING STRING IS A TAR PATH */

int string_contains_tar(char * string){

	return strstr(string,".tar")!=NULL;
}

/*CHECK IF THE CURRENT WORKING DIRECTORY IS INSIDE OF A TAR*/

int current_dir_is_tar(){

	if( getenv("tar")!=NULL ){

		if(strcmp(getenv("tar"),"")!=0){

			return 1;
		}

	}

	return 0;

}

/*CHECKS IF THE DIRECTORY PATH CONTAINS THE TAR FILE TAR*/

int tar_file_exists(char * path, char * tar){

	/*OPENING THE DIR PATH*/

	DIR *dirp = opendir(path);

	/*ERROR MANAGMENT*/

	if(dirp==NULL){

		perror("opendir");
		exit(-1);
	}

	struct dirent *entry;

	/*LOOPING ON THE DIRECTORY*/

	while((entry = readdir(dirp))){

		/*WE MAKE SURE TO IGNORE SPECIAL ENTRIES*/

		if(strcmp(".",entry->d_name)!=0 && strcmp("..",entry->d_name)!=0) {

			/*WE CHECK IF THE CURRENT ENTRY IS A TAR*/

			if(string_contains_tar(entry->d_name)){

				/*WE CHECK IF IT IS SPECFICIALLY TAR*/

				if (strcmp(entry->d_name,tar)==0){

					free(dirp);
					return 1;
				}
			}
		}
	}

	free(dirp);
	return 0;

}

/*RETURNS THE NAME OF THE TAR FILE WE ARE CURRENTLY WORKING ON*/

char * get_tar_name(){

	if(!current_dir_is_tar()){

		return NULL;
	}

	char ** tokens = decompose(getenv("tar"),"/");
	return tokens[0];

}

/*RETURNS THE NAME OF THE TAR FILE IN PATH*/

char * get_tar_name_file(char * path){

	char * copy=malloc(strlen(path)+sizeof(char));
	strcpy(copy,path);
	char ** tokens = decompose(copy,"/");
	return tokens[0];

}

/*	CAT THE STRINGS IN TOKENS WITH DELIMITER BETWEEN THEM INTO AN ONLY STRIN
EG IF TOKEN IS "a" "b" AND "c" AND DELIMITERS "/" IT RETURNS "a/b/c"*/

char * flatten(char ** tokens, char * delimiter){

	char *ret = malloc(strlen(tokens[0])+sizeof(char));
	strcpy(ret,tokens[0]);

	int cpt=1;

	while(tokens[cpt]!=NULL){

		ret =realloc(ret,strlen(ret)+(2*sizeof(char)));
		ret=strcat(ret,delimiter);
		ret=realloc(ret,strlen(ret)+strlen(tokens[cpt])+sizeof(char));
		ret=strcat(ret,tokens[cpt]);

		cpt++;

	}

	return ret;


}

/*RETURN THE ACTUAL PATH IN THE TAR BUT WITHOUT THE NAME OF THE TAR FILE AT THE BEGGINIGN*/

char * get_path_without_tar(){

	if(!current_dir_is_tar()){

		return NULL;
	}

	char ** tokens = decompose(getenv("tar"),"/");
	return flatten(&tokens[1],"/");
}

/*RETURN  PATH BUT WITHOUT THE NAME OF THE TAR FILE AT THE BEGGINIGN*/

char * get_path_without_tar_file(char * path){

	char *copy=malloc(strlen(path)+sizeof(char));
	strcpy(copy,path);
	char ** tokens = decompose(copy,"/");
	return flatten(&tokens[1],"/");
}

/*CHECKS IF THE FILE PATH EXIST IN THE TAR FILE TAR*/

int file_exists_in_tar(char * path, char * tar){


	// OPENING THE TAR FILE

	int fd=open(tar,O_RDONLY);

	//ERROR MANAGMENT

	if(fd==-1){
		print_error(NULL,tar,"does not exist");
		return 0;
	}

	// THIS IS WERE WE STORE HEADERS

	struct posix_header hd;

		do{

			// READING AN HEADER

			int rdcount=read(fd,&hd,BLOCKSIZE);

			//ERROR MANAGMENT

			if(rdcount<0){

				perror("reading tar file");
				close(fd);
				return 0;
			}


			//IF WE FOUND THE HEADER

			if(strcmp(hd.name,path)==0 && hd.typeflag=='5'){

				return 1;

			}

			//READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			unsigned int size;
			sscanf(hd.size, "%o",&size);

			//WE GET TO THE NEXT HEADER

			lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


		}while(hd.name[0]!='\0');

		return 0;

}

/*CHECKS IF THE FILE PATH EXIST IN THE TAR FILE TAR*/

int file_ndir_exists_in_tar(char * path, char * tar){


	// OPENING THE TAR FILE

	int fd=open(tar,O_RDONLY);

	//ERROR MANAGMENT

	if(fd==-1){
		print_error(NULL,tar,"does not exist");
		return 0;
	}

	// THIS IS WERE WE STORE HEADERS

	struct posix_header hd;

		do{

			// READING AN HEADER

			int rdcount=read(fd,&hd,BLOCKSIZE);

			//ERROR MANAGMENT

			if(rdcount<0){

				perror("reading tar file");
				close(fd);
				return 0;
			}


			//IF WE FOUND THE HEADER

			if(strcmp(hd.name,path)==0){

				return 1;

			}

			//READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			unsigned int size;
			sscanf(hd.size, "%o",&size);

			//WE GET TO THE NEXT HEADER

			lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


		}while(hd.name[0]!='\0');

		return 0;

}

/*IF PATH IS A VALID PATH, RETURN A CHAR * WITH THE PATH ( WITH .. PROCESSED ) ELSE
RETURNS NULL, IF WE SIMPLY NEED TO EXIT THE TAR, RETURNS ""*/

char * path_is_valid(char * path){

	/*WE COPY THE PATH TO AVOID BREAKING IT*/

	char* path_cpy= malloc(strlen(path)+sizeof(char));
	strcpy(path_cpy,path);

	/*WE DECOMPOSE THE PATH*/

	char ** tokens = decompose(path_cpy,"/");

	/*TRUE PATH WILL GIVE US

	A - IF WE STAY IN THE TAR, A PATH LOOKING LIKE getenv("tar")+PATH PROCESSED
	WITHOUT ANY ..

	B - IF WE GET OUT OF THE TAR AND NOTHING ELSE IT WILL RETURN "exit"

	C - IF WE NEED TO GET OUT OF THE TAR BUT THERE IS STILL A PATH TO PROCESS AFTER THAT
	IT WILL RETURN THE PATH TO PROCESS AFTER GETTING OUT OF THE TAR, .. INCLUDED

	D - IF THE PATH IS NOT VALID IT RETURNS NULL*/

	char * pathf=true_path(path);

	/*D - IF PATHF IS NULL WE SIMPLY RETURN IT AS IT MEANS THE PATH
	ISN'T VALID*/

	if(pathf==NULL){

		free(tokens);
		free(path_cpy);
		
		return pathf;
	}

	/*WE COPY PATHF AND THEN DECOMPOSE IT*/

	char * pathf_copy= malloc(strlen(pathf)+sizeof(char));
	strcpy(pathf_copy,pathf);
	char ** tokens_pathf=decompose(pathf_copy,"/");

	/*B - IF WE HAVE EXIT THEN WE SIMPLY EXIT THE TAR*/

	if(strcmp(pathf,"")==0){

		free(tokens);
		free(tokens_pathf);
		free(pathf_copy);
		free(path_cpy);

		return "";
	}

	/*C - WE STILL HAVE A PATH TO PROCESS BUT NOT IN THE TAR
	SO WE RETURN PATHF RIGHT AWAY*/

	if(!string_contains_tar(tokens_pathf[0])){

		free(tokens);
		free(tokens_pathf);
		free(pathf_copy);
		free(path_cpy);
		
		return pathf;
	}

	/*CHECKS THAT THE TAR FILE THE PATH IS IN IS IN THE CURRENT LAST NOT TAR DIRECTORY*/

	char bufdir [PATH_MAX + 1];
	getcwd(bufdir,sizeof(bufdir));

	if(!tar_file_exists(bufdir,tokens_pathf[0])){

		free(tokens);
		free(tokens_pathf);
		free(pathf_copy);
		free(path_cpy);

		return NULL;
	}

	/*IF THERE IS NO MORE TOKENS, AS WE JUST CHECKED THE EXISTENCE
	OF THE .TAR, WE CAN RETURN PATHF*/

	if(tokens_pathf[1]==NULL){

		free(tokens);
		free(pathf_copy);
		free(tokens_pathf);
		free(path_cpy);

		return pathf;
	}

	/*WE FLATTEN AND THEN MODIFY THE STRING SO WE CAN GET PATHF BUT WITHOUT
	THE XX.TAR TOKEN AT THE BEGGINING*/

	char * path_without_tar= flatten(&(tokens_pathf[1]),"/");
	path_without_tar=realloc(path_without_tar,strlen(path_without_tar)+2*sizeof(char));
	path_without_tar=strcat(path_without_tar,"/");;

	/*WE CHECK THAT THE PATH WE OBTAINED DOES EXIST IN THE TAR
	WE ARE CURRENTLY IN*/

	if(file_exists_in_tar(path_without_tar,tokens_pathf[0])){

		free(tokens);
		free(tokens_pathf);
		free(path_without_tar);
		free(pathf_copy);
		free(path_cpy);

		return pathf;
	}

	/*IF NOTHING FITS THEN WE RETURN NULL*/

	free(tokens);
	free(pathf);
	free(tokens_pathf);
	free(path_without_tar);
	free(pathf_copy);
	free(path_cpy);

	return NULL;
}



char * true_path(char * path){

	//copy the getenv into "tar"
  char * tar= malloc(strlen(getenv("tar"))+sizeof(char));

  if(tar==NULL){

  	perror("malloc");
  	exit(-1);
  }

  strcpy(tar,getenv("tar"));

	//decompose tar to browse throught it in "tokens"
  char ** tokens = decompose(tar,"/");

	//copy the path into "ar"
  char * ar =malloc(strlen(path)+sizeof(char));

  if(ar==NULL){

  	perror("malloc");
  	exit(-1);
  }

  strcpy(ar, path);

	//decompose ar to browe throught it in "tokens2"
  char ** tokens2 = decompose(ar,"/");

	//i and i2 are counter
  int i = 0;
  int i2 = 0;

	//we count the number of argument in tokens
  while (tokens[i] != NULL){

    i++;

  }

//while there are arguments in tokens2
  while (tokens2[i2] != NULL){

//if the arguments is a ".." we either remove a arguments of "tokens"
//or add a ".." in the end of the path
    if(strcmp(tokens2[i2],"..") == 0){

      if(i == 0||strcmp(tokens[i-1],"..")==0) {

      		tokens=realloc(tokens,(i+1)*sizeof(char *));
      		tokens[i]=tokens2[i2];
      		i++;
       }

      else{

      	tokens[i-1]=NULL;
      	i--;
      	tokens=realloc(tokens,i*sizeof(char *));

      }

    }
//if the argument is not a ".." we add the argument at the end
    else{

      tokens=realloc(tokens,(i+1)*sizeof(char *));
      tokens[i] = tokens2[i2];

      i++;
    }

    i2++;
  }
//we add a "NULL" at the end of "tokens"
  tokens=realloc(tokens,(i+1)*sizeof(char *));
  tokens[i] = NULL;

  if(i==0){
  	return "";
  }

//we flatten tokens to get the and return the path
  char *ret = flatten(tokens,"/");

  free(tokens);
  free(tokens2);
  free(tar);
  free(ar);

  return ret;
}


char ** tar_and_path(char *p){

	//We set a path removing every .. for argv1
	char * path1 = true_path(p);

	//Will be a counter
	int i2 = 0;

	//Array of the decompositiob of argv[1]
	char * path_1_copy = malloc(strlen(path1)+sizeof(char));
	strcpy(path_1_copy,path1);
	char ** tokens = decompose(path_1_copy,"/");


	//Will be the name of the tar to open
	char * tar=malloc(strlen("")+sizeof(char));
	strcpy(tar,"");

	//While we dont see a the name of the file strcat the path to the tar
	while(string_contains_tar(tokens[i2]) != 1){

		tar=realloc(tar,strlen(tar)+strlen(tokens[i2])+sizeof(char));
			strcat(tar,tokens[i2]);
		tar=realloc(tar,strlen(tar)+2*(sizeof(char)));
			strcat(tar,"/");
			i2++;
	}

	//Final strcat to cpy the name of the file
	tar=realloc(tar,strlen(tar)+strlen(tokens[i2])+sizeof(char));
	strcat(tar,tokens[i2]);
	i2++;

	char * path = NULL;
	if(tokens[i2] != 0)
		path = flatten(&(tokens[i2]),"/");

	free(path_1_copy);
	free(tokens);
	free(path1);
	//We make a " char ** tokens3" where we stock "tar" in tokens3[0]
	//and "path" in tokens3[1];
	char ** tokens3= (char **) calloc(3,sizeof(char *));


	tokens3[0] = tar;
	tokens3[1] = path;
	tokens3[2]=NULL;


	return tokens3;
}

//COPY A FILE FROM A TAR TO A REP OUTSIDE THE TAR, MODIFIED VERSION FOR REDIRECTIONS

int tar_vers_ext_cp(char *argv[]){

   struct posix_header hd; //Header for the tar

  unsigned int size; //Size of the file that will be initialized later

  //we get the tar to open and the path for the file
  //from tar_and_path
  char ** arg = tar_and_path(argv[1]);

	if (arg[0] == NULL){
		print_error("cp : '",argv[1],"' Problem with argv 1");
		exit(-1);
	}

	if (arg[1] == NULL){
		print_error(NULL,argv[1],"' Problem with argv 1");
		exit(-1);
	}

  char * tar = malloc(strlen(arg[0])+sizeof(char));
  strcpy (tar,arg[0]);
  char * path = malloc(strlen(arg[1])+sizeof(char));
  strcpy (path,arg[1]);

  free(arg[0]);
  free(arg[1]);
  free(arg);

  // OPENING THE TAR FILE

  int fd=open(tar,O_RDWR);
  free(tar);
  //ERROR MANAGMENT

  if(fd==-1){
    print_error(NULL,tar,"open tar file");
    exit(-1);
  }

  // THIS LOOP ALLOWS US TO LOOK FOR THE HEADER CORRESPONDING TO THE FILE WE WANT TO
  // FIND IN THE TAR

  do{
    // READING AN HEADER

    int rdcount=read(fd,&hd,BLOCKSIZE);

    //ERROR MANAGMENT

    if(rdcount<0){

      print_error(NULL,tar,"' reading tar file");
      close(fd);
      free(path);
      return -1;
    }

    //IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE GOOD HEADER

    if((hd.name[0]=='\0')){
      free(path);
      return -2;
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
  
  free(path);
  //CREATING THE FILE TO COPY

  //Finding the right permission
  int a = 0;
		switch(hd.mode[4]) {
  			case '0': ; break;
  			case '1': a =S_IXUSR; break;
  			case '2': a =S_IWUSR; break;
  			case '3': a =S_IXUSR | S_IWUSR; break;
  			case '4': a =S_IRUSR; break;
  			case '5': a = S_IRUSR | S_IXUSR; break;
  			case '6': a = S_IRUSR | S_IWUSR;break;
  			case '7': a = S_IRWXU; break;
		}

    int b = 0;
  		switch(hd.mode[5]) {
    			case '0': ; break;
    			case '1': b =S_IXGRP; break;
    			case '2': b =S_IWGRP; break;
    			case '3': b =S_IXGRP | S_IWGRP; break;
    			case '4': b =S_IRGRP; break;
    			case '5': b = S_IRGRP | S_IXGRP; break;
    			case '6': b = S_IRGRP | S_IWGRP; break;
    			case '7': b = S_IRWXG; break;
  		}

      int c = 0;
    		switch(hd.mode[6]) {
      			case '0': ; break;
      			case '1': c =S_IXOTH; break;
      			case '2': c =S_IWOTH; break;
      			case '3': c =S_IXOTH | S_IWOTH; break;
      			case '4': c =S_IROTH; break;
      			case '5': c = S_IROTH | S_IXOTH; break;
      			case '6': c = S_IROTH | S_IWOTH;break;
      			case '7': c = S_IRWXO; break;
    		}

	  int fd2;

	//if the file is a FIFO
	if(hd.typeflag == 54){
		fd2 = mkfifo(true_path(argv[2]), a | b |c );
		fd2 = open(true_path(argv[2]), O_RDWR | O_CREAT , a | b | c);
	}

	//if the file is a LINK
	else if(hd.typeflag == 50){
		char * e = malloc(strlen(hd.linkname) + sizeof(char));
		strcpy(e,hd.linkname);
		char * f = malloc(strlen(true_path(argv[2])) + sizeof(char));
		strcpy(f,true_path(argv[2]));
		symlink(e, f);

			fd2=open(f, O_RDWR | O_CREAT , a | b | c);
			free (e);
			free (f);

	}
	else {
		fd2=open(argv[2], O_RDWR | O_CREAT , a | b | c);

	}

	fchmod(fd2, a | b | c);


	if(fd2 < 0){
		print_error(NULL,argv[2],"error argv[2]");
		return -1;
	}


  //GETTING THE SIZE OF WHAT WE NEED TO READ

	sscanf(hd.size, "%o",&size);
	char rd [size];


	    int rdtmp = read(fd, rd, size );


	    //EROR MANAGMENT

	    if(rdtmp<0){

	      print_error(NULL,tar,"' Reading tar file");
	      exit(-1);
	    }

	    //WRITING THE BLOCK AND ERROR MANGEMENT

	    if(write(fd2,rd, size)<0){

	      print_error(NULL, argv[2] ," Writing file content");
	      exit(-1);

	    }

  //CLOSING WRITING FILE


  close(fd);

  close(fd2);

return 0;
}

void set_checksum_cp(struct posix_header *hd) {
  memset(hd->chksum,' ',8);
  unsigned int sum = 0;
  char *p = (char *)hd;
  for (int i=0; i < BLOCKSIZE; i++) { sum += p[i]; }
  sprintf(hd->chksum,"%06o",sum);
}

/* Check that the checksum of a header is correct */

int check_checksum_cp(struct posix_header *hd) {
  unsigned int checksum;
  sscanf(hd->chksum,"%o ", &checksum);
  unsigned int sum = 0;
  char *p = (char *)hd;
  for (int i=0; i < BLOCKSIZE; i++) { sum += p[i]; }
  for (int i=0;i<8;i++) { sum += ' ' - hd->chksum[i]; }
  return (checksum == sum);
}

//COPY A FILE FROM OUTSIDE OF THE TAR  INTO A TAR
int ext_vers_tar_cp(char *argv[]){

  char buf [BLOCKSIZE];

  struct posix_header hd; //Header for the tar

  unsigned int size; //Size of the file that will be initialized later

  //we get the tar to open and the path for the file
  //from tar_and_path
  char ** arg = tar_and_path(argv[2]);

  char * tar = malloc(strlen(arg[0])+sizeof(char));
  strcpy (tar,arg[0]);
  char * path = malloc(strlen(arg[1])+sizeof(char));
  strcpy (path,arg[1]);

  free(arg[0]);
  free(arg[1]);
  free(arg);

  // OPENING THE TAR FILE

  int fd=open(tar,O_RDWR);
	free(tar);

  if(fd <0){
    print_error(NULL,argv[2],"Problem with argv 2");
    exit(-1);
  }

  int fd2 = open(argv[1], O_RDONLY);

  if(fd2<0){
    print_error(NULL, argv[1] ,"Aucun fichier ou dossier de ce type\n");
    return -1;
  }


  // THIS LOOP ALLOWS US TO GO TO THE END OF THE TAR
  do{

    // READING AN HEADER

    int rdcount=read(fd,&hd,BLOCKSIZE);

    //ERROR MANAGMENT

    if(rdcount<0){
      print_error(NULL,tar," reading tar file");
      close(fd);
      exit (-1);
    }

    //IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE HEADER THEN IT DOESNT EXIST AND WE CAN CREATE IT

    if(strcmp(hd.name,path) == 0){
      	return -1;
    }

    if((hd.name[0]=='\0')){
      break;
    }


    //READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

    sscanf(hd.size, "%o",&size);

    //WE GET TO THE NEXT HEADER

    lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(hd.name[0]!='\0');//While the header is not at the end block of 0


  struct posix_header temporaire;//The 'entete' we will put at the end of the tar


  //INIT STRUCT MEMORY

  memset(&temporaire,0,BLOCKSIZE);

  //Naming the file as argv[3]

  sprintf(temporaire.name,"%s",path);
  free(path);


  //FILLING MODE
  struct stat f;
  stat(argv[1],&f);
  sprintf(temporaire.mode,"%7o", f.st_mode);


 //SIZE BECOME THE SIZE OF THE COPIED FILE

  unsigned int fsize;//Size of the file that will be initialized later
  fsize = lseek(fd2,0,SEEK_END);//GET FILE SIZE


  //SETTING THE ENTETE SIZE AS THE SAME OF THE COPIED FILE

  sprintf(temporaire.size,"%011o",fsize);
  lseek(fd2,0,SEEK_SET);//RESETING fd2 OFFSET

  // LAST MODIFICATION DATE

  unsigned int  t_acc= time(NULL);
  sprintf(temporaire.mtime ,"%o", t_acc);


  //FILLING MAGIC FIELD


 sprintf(temporaire.magic,TMAGIC);

//Typeflag
 if(S_ISREG(f.st_mode)){
	 temporaire.typeflag = '0';
	 }
//LINKNAME
 if(S_ISLNK(f.st_mode)){
	 temporaire.typeflag = '2';
 }
 if(S_ISCHR(f.st_mode) != 0){
	 temporaire.typeflag = '3';
 }
 if(S_ISBLK(f.st_mode) != 0){
	 temporaire.typeflag = '4';
 }
 if(S_ISFIFO(f.st_mode) != 0){
	 temporaire.typeflag = 54 ;
 }

 // VERSION

 sprintf(temporaire.version,TVERSION);

 //USER NAME

 getlogin_r(temporaire.uname, sizeof(temporaire.uname));

 //GETTING GROUP NAME

 gid_t gid= getgid();
 struct group * g= getgrgid(gid);

 //ERROR MANAGMENT

 if(g==NULL){
   print_error(NULL,NULL,"Reading group ID");
   exit(-1);
 }
 sprintf(temporaire.gname,"%s",g->gr_name);

 //SETTING CHECKSUM ONCE ALL THE OTHER FIELDS ARE FILLED

 set_checksum_cp(&temporaire);
 check_checksum_cp(&temporaire);

 // SINCE WE READ THE FIRST EMPTY BLOCK (TAR ENDS WITH 2 EMPTY BLOCKS) TO CHECK IF WE HAD READ ALL OF THE HEADERS
 // WE NEED TO GO BACK ONE BLOCK

 lseek(fd,-512,SEEK_CUR);

 //WRITING THE NEW DIRECTORY AT THE END OF THE FILE


  int rddd=write(fd,&temporaire,BLOCKSIZE);

  if(rddd<BLOCKSIZE){

    print_error(NULL,NULL,"Error writing in file");
    exit(-1);
  }


  char buff [BLOCKSIZE];
	memset(buff,0,BLOCKSIZE);
  int rdtmp=0;

  do{
      rdtmp = read(fd2,buff, BLOCKSIZE);
      //EROR MANAGMENT

      if(rdtmp<0){
        print_error(NULL,NULL,"Reading tar file");
        exit(-1);
      }

      //WRITING THE BLOCK AND ERROR MANGEMENT

      if(write(fd,buff, BLOCKSIZE)<0){

        print_error(NULL,NULL,"Writing file content");
        exit(-1);

      }
    memset(buff,0,BLOCKSIZE);
  }while(rdtmp>0);
    //RESETING THE BUFFER


  memset(buf,0,BLOCKSIZE);

  for(int i=0;i<2;i++){

    int rdd=write(fd,buf,BLOCKSIZE);


    if(rdd<BLOCKSIZE){

      print_error(NULL,NULL,"Error writing in file2");
      exit(-1);
    }

  }

  lseek(fd,0,SEEK_SET);
  close (fd);
  close (fd2);
  return 0;
}
