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

// Format: ./rmdir fichiertar.tar directory otherdirectory ...
// (Un repertoire doit avoir '/' a la fin)

int rmtar(char *argv){

	// ======================================================================
	// 			      OUVERTURE DU TAR
	// ======================================================================

	char * tar = getenv("tar");
	char * var_rep = NULL;

	// Si la variable d'nevironnement est dans un repertoire du tar
	if(strchr(tar, '/') != NULL) {
		strtok(tar, "/");
		char * var_r = strtok(NULL, "");
		var_rep = malloc(strlen(var_r) + 2);
		strcpy(var_rep, var_r);
		strcat(var_rep, "/");
	}

	// Ouverture du tar
	int fd = open(tar, O_RDWR);
	if(fd < 0){
		perror("\033[1;31mErreur lors de l'ouverture du tar\033[0m");
		exit(-1);
	}

	struct posix_header * p_hdr;
	char tampon[512];

	// ======================================================================
	// 	 	    PARCOURS DU TAR POUR CHAQUE ARGUMENT
	// ======================================================================



		int valide = 0;			// 0: fichier ne peut pas etre supprime
						// 1: le fichier peut etre supprime

		char * fich = NULL;		// Nom du fichier
		int rep = 0;			// c'est un repertoire
		unsigned int size;		// Taille du fichier
		off_t longueur = 0;		// Somme de la taille des fichiers avant le repertoire
		off_t supp = 0;			// Somme de la taille du repertoire et de ses fichiers
		off_t dep = 0;			// Somme de la taille des fichiers apres le repertoire
		char * arg;				// argv[i] adapte au tar et la variable d'environnement

		// On evite "-r"
		if(strcmp(argv, "-r") != 0) {

			// ----------------------------------------------------------------------
			// 	 	     		FICHIER A SUPPRIMER
			// ----------------------------------------------------------------------

			// Si un repertoire est entre sans '/'
			arg = malloc(strlen(argv) + 1);
			strcpy(arg,argv);

			// Si la variable d'environnement se trouve dans un repertoire du tar
			if (var_rep != NULL) {
				char * nouv_arg = malloc(strlen(var_rep) + strlen(arg) + 1);
				strcpy(nouv_arg, var_rep);
				strcat(nouv_arg, arg);
				arg = realloc(arg, strlen(var_rep) + strlen(arg) + 1);
				strcpy(arg, nouv_arg);
			}

			// ----------------------------------------------------------------------
			// 	 		       PARCOURS DU TAR
			// ----------------------------------------------------------------------

			while(1) {

				// Lecture du bloc

				int rdcount = read(fd,&tampon, BLOCKSIZE);
				if(rdcount<0){
					perror("\033[1;31mErreur lors de la lecture du tar\033[0m");
					close(fd);
					exit(-1);
				}

				// Extraction des informations

				p_hdr = (struct posix_header*)tampon;
				sscanf(p_hdr->size, "%o",&size);

				// On arrive a la fin au bloc nul

				if(strlen(p_hdr-> name) == 0) {
					if (fich == NULL) {
						printsss("rm: impossible de supprimer '", argv, "': Aucun fichier ou dossier de ce type\n");
						break;
					}
					else {
						if(rep == 1) {
							printsss("rm: impossible de supprimer '", argv, "': est un dossier\n");
							break;
						}
						else {
							valide = 1;
							dep = dep + BLOCKSIZE;
							break;
						}
					}
				}

				// Si on trouve le fichier

				char * name;

				// Si l'argument entree est un repertoire sans '/' a la fin
				if(p_hdr-> typeflag == '5' && arg[strlen(arg) - 1] != '/') {
					name = malloc(strlen(arg) + 2);
					strcpy(name, arg);
					strcat(name, "/");
				}
				// Sinon on ne change pas arg
				else {
					name = malloc(strlen(arg) + 1);
					strcpy(name, arg);
				}
				
				// Comparaison
				if(strcmp(p_hdr -> name, name) == 0) {
					if(p_hdr-> typeflag == '5')
						rep = 1;
					fich = malloc(strlen(p_hdr->name) + 1);
					strcpy(fich, p_hdr->name);
				}

				// Stockage des octets a utiliser lors de la suppresion

				if(fich != NULL) {
					// Le fichier a supprimer
					if(strcmp(p_hdr -> name, arg) == 0)
						supp = supp + BLOCKSIZE + (((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE);

					// Toutes donnees se situant apres le repertoire
					else
						dep = dep + BLOCKSIZE + (((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE); 
				}
				// Toutes donnees avant d'avoir trouve le repertoire
				else 
					longueur = longueur + BLOCKSIZE + (((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE);

				// On passe a l'entete suivante

				lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);

			}

			// ----------------------------------------------------------------------
			// 	 		  TRAITEMENT DU REPERTOIRE
			// ----------------------------------------------------------------------

			if (valide == 1) {

				// On stocke les donnees a deplacer apres la suppression du repertoire
				// Rappel : le 1er bloc nul a ete lu dans la boucle

				lseek(fd, -dep, SEEK_CUR);
				dep = dep + BLOCKSIZE;
				char mem[dep];

				int rd = read(fd, &mem, dep);
				if(rd<0){
					perror("\033[1;31mErreur lors de la lecture du tar\033[0m");
					close(fd);
					exit(-1);
				}

				// On supprime le repertoire

				lseek(fd, longueur, SEEK_SET);
				int wr = write(fd, &mem, dep);
				if(wr<0){
					perror("\033[1;31mErreur lors de l'écriture du tar\033[0m");
					close(fd);
					exit(-1);
				}

				ftruncate(fd, longueur+dep);
			}

			// Retour au depart

			lseek(fd,0,SEEK_SET);
			
		}

	

	close(fd);
	exit(0);
}

//COPY A FILE FROM A TAR INTO THE CURRENT REPERTORY
//the format is -- cp nameoftar.tar path/of/file name_of_copy
int tar_vers_ext(char *argv[]){
   prints("tar vers ext\n");
  
  struct posix_header hd; //Header for the tar

  unsigned int size; //Size of the file that will be initialized later
  
  char * tar=get_tar_name("tar");
  
  // Si la variable d'nevironnement est dans un repertoire du tar
	if(strchr(tar, '/') != NULL) {
	  char * var_rep = NULL;
		strtok(tar, "/");
		char * var_r = strtok(NULL, "");
		var_rep = malloc(strlen(var_r) + 2);
		strcpy(var_rep, var_r);
		strcat(var_rep, "/");
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

  //CREATING THE FILE TO COPY

  int fd2=open(argv[2], O_WRONLY | O_CREAT , S_IRUSR | S_IWUSR);

  //GETTING THE SIZE OF WHAT WE NEED TO READ

  char rd [BLOCKSIZE] ;

  //LOOP TO READ AND WRITE THE BLOCKS OF THE FILE CONTENT

    for(unsigned int i=0; i<(size+ BLOCKSIZE - 1);i++){
    
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
  
return 0;
}


//COPY A FILE FROM THE CURRENT DIRECTORY  INTO THE TAR
//the format is -- cp  path/of/file name_of_the_tar.tar name_of_copy
int ext_vers_tar(char *argv[]){
  	prints("ext vers tar\n");
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

 for(unsigned int i=0; i<(fsize+ BLOCKSIZE - 1);i++){
 
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

    memset(buff, 0, 1);
  }

    
 
  memset(buf,0,BLOCKSIZE);
  
  for(int i=0;i<2;i++){

    int rdd=write(fd,buf,BLOCKSIZE);


    if(rdd<BLOCKSIZE){

      perror("Error writing in file2");
      exit(-1);
    }

  }
 
  char tm[100];
  strcpy(tm,"rm ");
  strcat(tm, argv[1]);
  system(tm);
  
  lseek(fd,0,SEEK_SET);    
  close (fd);
  close (fd2);
  
  return 0;
}

//COPY A FILE FROM A TAR INTO ANOTHER TAR
//the format is -- cp nameoftar.tar path/of/file nameof2tar.tar name_of_copy
int tar_vers_tar(char *argv[]){
 prints("tar vers tar\n");
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
      
      break;
    }
    
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

  if (argc == 3){
    struct posix_header hd; //Header for the tar

    unsigned int size; //Size of the file that will be initialized later
    
     
      if (tar_vers_tar(argv)==0){
	rmtar(argv[1]);
	return 0;
      }
 
      char * tar=get_tar_name("tar");

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

	  break;
	}

	//READONG THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			
	sscanf(hd.size, "%o",&size);

	//If the file is already in the tar we call the function tar_vers_ext

	if(strcmp(getenv("tar"),tar)==0){
	  if(strcmp(hd.name,argv[1])==0){
	    int fg = open(argv[2],O_RDONLY);
	    if (fg < 0){
	     
	      tar_vers_ext(argv);
	      rmtar(argv[1]);
	      return 0;
	    }
	  }
	   

	}

	//OTHERWISE WE GET TO THE NEXT HEADER

	lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


      }while(strcmp(hd.name,argv[1])!=0);
   
      int fs = open(argv[1],O_RDONLY);

      if (fs > 0){

	ext_vers_tar(argv);
	
	return 0;
      }
      else{
	prints("The file already exists in to the path of the copy \n");
	return 0;
      }
 
    }
  
  else{
    prints("cp needs 2 argument of the format cp copied_file paste_file \n");
    return 0;
  }
  return 0;
}
