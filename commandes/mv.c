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
#include <limits.h>
#include "lib.h"
int rmtar(char *argv){

	// ======================================================================
	// 	 			INITIALISATION
	// ======================================================================



	// S'il y a l'option "-r"
	int r = 0;

	// ======================================================================
	// 			      OUVERTURE DU TAR
	// ======================================================================

	struct posix_header * p_hdr;
	char tampon[512];
	int fd;
	// ======================================================================
	// 	 	    PARCOURS DU TAR POUR CHAQUE ARGUMENT
	// ======================================================================



		//we get the tar to open and the path for the file
		//from tar_and_path
		char ** ar = tar_and_path(argv);

 	 char * tar = malloc(strlen(ar[0])+sizeof(char));
 	 strcpy (tar,ar[0]);
 	 char * path = malloc(strlen(ar[1])+sizeof(char));
 	 strcpy (path,ar[1]);
	 free(ar);
	  // OPENING THE TAR FILE
	  fd=open(tar,O_RDWR);
		free(tar);

	  if(fd < 0){
	    perror("\033[1;31mErreur lors de l'ouverture du tar\033[0m");
	    exit(-1);
	  }

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
			arg = malloc(strlen(path) + 1);

			strcpy(arg,path);

			free(path);

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
						print_error("rm: impossible de supprimer '", argv, "': Aucun fichier ou dossier de ce type\n");
						break;
					}
					else {
						if(rep == 1 && r == 0) {
							print_error("rm: impossible de supprimer '", argv, "': est un dossier\n");
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
					// Le repertoire et ses fichiers a supprimer
					if(r == 1 && estDansRep(p_hdr-> name, fich) == 1)
						supp = supp + BLOCKSIZE + (((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE);

					// Le fichier a supprimer
					else if(r == 0 && strcmp(p_hdr -> name, arg) == 0)
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
			close(fd);

		}

	free(arg);
	close(fd);
	exit(0);
}

//COPY A FILE FROM A TAR TO A REP OUTSIDE THE TAR
int tar_vers_ext(char *argv[]){

  struct posix_header hd; //Header for the tar

  unsigned int size; //Size of the file that will be initialized later

  //we get the tar to open and the path for the file
  //from tar_and_path
  char ** arg = tar_and_path(argv[1]);

  char * tar = malloc(strlen(arg[0])+sizeof(char));
  strcpy (tar,arg[0]);
  char * path = malloc(strlen(arg[1])+sizeof(char));
  strcpy (path,arg[1]);

  free(arg);

  // OPENING THE TAR FILE

  int fd=open(tar,O_RDWR);
  free(tar);
  //ERROR MANAGMENT

  if(fd==-1){
    print_error("cp : '",tar,"' open tar file");
    exit(-1);
  }

  // THIS LOOP ALLOWS US TO LOOK FOR THE HEADER CORRESPONDING TO THE FILE WE WANT TO
  // FIND IN THE TAR

  do{
    // READING AN HEADER

    int rdcount=read(fd,&hd,BLOCKSIZE);

    //ERROR MANAGMENT

    if(rdcount<0){

      print_error("cp : '",tar,"' reading tar file");
      close(fd);
      return -1;
    }

    //IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE GOOD HEADER

    if((hd.name[0]=='\0')){
      print_error("cp: impossible d'évaluer '", path ,"' : Aucun fichier ou dossier de ce type\n");
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

	}
	else {
		fd2=open(true_path(argv[2]), O_RDWR | O_CREAT , a | b | c);

	}
	fchmod(fd2, a | b | c);


	if(fd2 < 0){
		print_error("cp : ",argv[2],"error argv[2]");
		return -1;
	}


  //GETTING THE SIZE OF WHAT WE NEED TO READ

	sscanf(hd.size, "%o",&size);
	char rd [size];


	    int rdtmp = read(fd, rd, size );


	    //EROR MANAGMENT

	    if(rdtmp<0){

	      print_error("cp  '",tar,"' Reading tar file");
	      exit(-1);
	    }

	    //WRITING THE BLOCK AND ERROR MANGEMENT

	    if(write(fd2,rd, size)<0){

	      print_error("cp ", argv[2] ," Writing file content");
	      exit(-1);

	    }

  //CLOSING WRITING FILE


  close(fd);

  close(fd2);

return 0;
}

//COPY A FILE FROM OUTSIDE OF THE TAR  INTO A TAR
int ext_vers_tar(char *argv[]){

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

  free(arg);

  // OPENING THE TAR FILE

  int fd=open(tar,O_RDWR);
	free(tar);

  if(fd <0){
    print_error("cp : '",argv[2],"' Problem with argv 2");
    exit(-1);
  }

  int fd2 = open(true_path(argv[1]), O_RDONLY);

  if(fd2<0){
    print_error("cp: impossible d'évaluer ", argv[1] ," : Aucun fichier ou dossier de ce type\n");
    exit(-1);
  }


  // THIS LOOP ALLOWS US TO GO TO THE END OF THE TAR
  do{

    // READING AN HEADER

    int rdcount=read(fd,&hd,BLOCKSIZE);

    //ERROR MANAGMENT

    if(rdcount<0){
      print_error("cp : ",tar," reading tar file");
      close(fd);
      return -1;
    }

    //IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE HEADER THEN IT DOESNT EXIST AND WE CAN CREATE IT

    if(strcmp(hd.name,path) == 0){
      break;
    }

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
  free(path);


  //FILLING MODE
  struct stat f;
  stat(true_path(argv[1]),&f);
 	sprintf(temporaire.mode,"%7o", f.st_mode);

	//LINKNAME OF THE FILE IS COPY

	//struct stat g;

  //strcpy(temporaire.linkname,lstat(true_path(argv[1]),&g));


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
 if(S_ISREG(f.st_mode) != 0){
	 temporaire.typeflag = 48;
	 }
 if(S_ISLNK(f.st_mode) != 0){
	 temporaire.typeflag = 50;
 }
 /*if(S_ISCHR(f.st_mode) != 0){
	 temporaire.typeflag = '3';
 }
 if(S_ISBLK(f.st_mode) != 0){
	 temporaire.typeflag = '4';
 }*/
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

 set_checksum(&temporaire);
 check_checksum(&temporaire);

 // SINCE WE READ THE FIRST EMPTY BLOCK (TAR ENDS WITH 2 EMPTY BLOCKS) TO CHECK IF WE HAD READ ALL OF THE HEADERS
 // WE NEED TO GO BACK ONE BLOCK

 lseek(fd,-512,SEEK_CUR);

 //WRITING THE NEW DIRECTORY AT THE END OF THE FILE


  int rddd=write(fd,&temporaire,BLOCKSIZE);

  if(rddd<BLOCKSIZE){

    print_error(NULL,NULL,"Error writing in file");
    exit(-1);
  }


  char buff [fsize];

    int rdtmp = read(fd2,buff, fsize);
    //EROR MANAGMENT

    if(rdtmp<0){
      print_error(NULL,NULL,"Reading tar file");
      exit(-1);
    }

    //WRITING THE BLOCK AND ERROR MANGEMENT

    if(write(fd,buff, fsize)<0){

      print_error(NULL,NULL,"Writing file content");
      exit(-1);

    }

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


//COPY A FILE FROM A TAR INTO ANOTHER TAR
int tar_vers_tar(char *argv[]){

  //HEADER FOR THE FIRST TAR WE COPY THE FILE
  struct posix_header hd;

  //HEADER FOR THE SECOND TAR WE COPIED FILE WILL BE
  struct posix_header hd2;

  //we get the tar to open and the path for the file
  //from tar_and_path but since there are 2 different tar
  //we do it 2 time for the copied file and the paste one

  //Size of the file
  unsigned int size;
  unsigned int size2;

  char ** arg = tar_and_path(argv[1]);

  char * tar = malloc(strlen(arg[0])+sizeof(char));
  strcpy (tar,arg[0]);
  char * path = malloc(strlen(arg[1])+sizeof(char));
  strcpy (path,arg[1]);

  free(arg);

  char ** arg2 = tar_and_path(argv[2]);

  char * tar2 = malloc(strlen(arg2[0])+sizeof(char));
  strcpy (tar2,arg2[0]);
  char * path2 = malloc(strlen(arg2[1])+sizeof(char));
  strcpy (path2,arg2[1]);

  free(arg2);

  int fd = open(tar,O_RDWR);
	if(fd < 0){
		print_error("cp : '",tar,"' error opening with first tar");
		return -1;
	}
 	free(tar);

  int fd2= open(tar2,O_RDWR);
	if(fd2 < 0){
		print_error("cp : '",tar2,"' error opening with second tar");
	}
  free(tar2);


  char rd [BLOCKSIZE] ;
  //OPENING FIRST TAR AND FINDING FILE
  do{

    // READING AN HEADER

    int rdcount=read(fd,&hd,BLOCKSIZE);

    //ERROR MANAGMENT

    if(rdcount<0){
      print_error("cp :",NULL,"reading tar file\n");
      close(fd);
      return -1;
    }

    //IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE GOOD HEADER

    if((hd.name[0]=='\0')){
      print_error("cp: impossible d'évaluer '", path ,"' : Aucun fichier ou dossier de ce type \n");
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
      print_error("cp :",NULL,"reading tar file");
      close(fd);
      return -1;
    }

    //IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE HEADER THEN IT DOESNT EXIST AND WE CAN CREATE IT

    if(strcmp(hd2.name,path2) == 0){
      break;
    }

    if((hd2.name[1]=='\0')){
      break;
    }


    //READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

    sscanf(hd2.size, "%o",&size2);

    //WE GET TO THE NEXT HEADER

    lseek(fd2,((size2+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(hd2.name!=0);
	sscanf(hd.size, "%o",&size);


   //Header we will put at the end of the second tar
  struct posix_header temporaire;


//INIT STRUCT MEMORY

 memset(&temporaire,0,BLOCKSIZE);

 // CHOOSE NAME FILE
 sprintf(temporaire.name,"%s",path2);
 free(path2);
  //FILLING MODE


 strcpy(temporaire.mode,hd.mode);

	//LINKNAME OF THE FILE IS COPY

  strcpy(temporaire.linkname,hd.linkname);

 //SIZE IS FILE OF COPIED FILE

 sprintf(temporaire.size,"%s",hd.size);

  //FILLING MAGIC FIELD

 sprintf(temporaire.magic,TMAGIC);

 // LAST MODIFICATION DATE

 unsigned int  t_acc= time(NULL);
 sprintf(temporaire.mtime ,"%o", t_acc);


 //FILE SO TYPE IS 0

 temporaire.typeflag=hd.typeflag;


 // VERSION

 sprintf(temporaire.version,TVERSION);

 //USER NAME

 getlogin_r(temporaire.uname, sizeof(temporaire.uname));

 //GETTING GROUP NAME

 gid_t gid= getgid();
 struct group * g= getgrgid(gid);

 //ERROR MANAGMENT

 if(g==NULL){
   print_error("cp :",NULL,"Reading group ID");
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

    print_error("cp :",NULL,"Error writing in file");
    exit(-1);
  }


  char buff [BLOCKSIZE];


	//writing the file
  for(unsigned int i=0; i<((size+ BLOCKSIZE - 1) >> BLOCKBITS);i++){

    int rdtmp = read(fd, buff, BLOCKSIZE);


    if(rdtmp<0){
      print_error("cp : '",tar,"' Reading tar file");
      exit(-1);
    }

    //WRITING THE BLOCK AND ERROR MANGEMENT

    if(write(fd2,buff, BLOCKSIZE)<0){

      print_error("cp :",NULL,"Writing file content");
      exit(-1);

    }


   memset(rd, 0, BLOCKSIZE);

  }
		rmtar(path);
	  free(path);


  memset(buff,0,BLOCKSIZE);
	//writing the 2 last tar block
   for(int i=0;i<2;i++){

    int rdd=write(fd2,buff,BLOCKSIZE);


    if(rdd<BLOCKSIZE){

      print_error("cp :",NULL,"Error writing in file");
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
  char *test = malloc(strlen(argv[1])+sizeof(char));
  strcpy (test,argv[1]);

  //Get a variable containing argv[2]
  char *test2 = malloc(strlen(argv[2])+sizeof(char));
  strcpy (test2,argv[2]);

  //We set a path removing every .. for argv1
  char * path1 = malloc(strlen(true_path(test))+sizeof(char));
  strcpy(path1,true_path(test));

  //We set a path removing every .. for argv2
  char * path2 = malloc(strlen(true_path(test2))+sizeof(char));
  strcpy(path2,true_path(test2));

  free(test);
  free(test2);


  if (argc == 3){

    //if argv1 is not inside a tar and argv2 is insiede a tar call ext_vers_tar
    if((string_contains_tar(path1) == 0) && (string_contains_tar(path2) == 1)){
      ext_vers_tar(argv);
				remove(path1);
    }

    //if argv1 is inside a tar and argv2 is not inside a tar call _tar_vers_ext
    if((string_contains_tar(path1) == 1) && (string_contains_tar(path2) == 0)){
      tar_vers_ext(argv);
    }

    //if argv1 and argv2 are inside tar then call tar_vers_tar
    if((string_contains_tar(path1) == 1) && (string_contains_tar(path2) == 1)){
    	tar_vers_tar(argv);
    }

    free(path1);
    free(path2);
    return 0;
  }
	else if (argc < 3){
		free(path1);
    free(path2);
		print_error("cp: ",NULL," opérande de fichier manquant \n");
		return -1;
	}
	free(path1);
	free(path2);
  return 0;
}
