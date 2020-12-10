#include <stdio.h>		// sscanf perror
#include <sys/types.h>		// lseek ftruncate
#include <fcntl.h>		// open
#include <unistd.h>		// read close lseek write ftruncate
#include <string.h>		// strcat strcmp
#include <stdlib.h>		// exit
#include "tar.h"
#include "print.h"
#include "lib.h"
#include "tar_nav.h"

// Format: ./rmdir fichiertar.tar directory otherdirectory ...
// (Un repertoire doit avoir '/' a la fin)

int main(int argc, char *argv[]){

	// ======================================================================
	// 	 			INITIALISATION
	// ======================================================================

	// Conditions des valeurs d'entrée
	if (argc < 2){
		prints("rmdir: opérande manquant\n");
		exit(-1);
	}

	// S'il y a l'option "-r"
	int r = 0;
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-r") == 0)
			r = 1;
	}

	// ======================================================================
	// 			      OUVERTURE DU TAR
	// ======================================================================

	struct posix_header * p_hdr;
	char tampon[512];
	int fd;
	// ======================================================================
	// 	 	    PARCOURS DU TAR POUR CHAQUE ARGUMENT
	// ======================================================================

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
	  fd=open(tar,O_RDWR);


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
		if(strcmp(argv[i], "-r") != 0) {

			// ----------------------------------------------------------------------
			// 	 	     		FICHIER A SUPPRIMER
			// ----------------------------------------------------------------------

			// Si un repertoire est entre sans '/'
			arg = malloc(strlen(path) + 1);
			strcpy(arg,path);
			prints(arg);

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
						printsss("rm: impossible de supprimer '", argv[i], "': Aucun fichier ou dossier de ce type\n");
						break;
					}
					else {
						if(rep == 1 && r == 0) {
							printsss("rm: impossible de supprimer '", argv[i], "': est un dossier\n");
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

	}
	close(fd);		
	exit(0);
}
