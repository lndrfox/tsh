#include <stdio.h>		// sscanf perror
#include <sys/types.h>	// lseek ftruncate
#include <fcntl.h>		// open
#include <unistd.h>		// read close lseek write ftruncate unlink
#include <string.h>		// strcat strcmp
#include <stdlib.h>		// exit
#include "tar.h"
#include "print.h"
#include "lib.h"
#include "tar_nav.h"

int main(int argc, char *argv[]){

	// ======================================================================
	// 	 			INITIALISATION
	// ======================================================================

	// S'il y a l'option "-r"
	int r = 0;
	int nbfich = 0;
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-r") == 0)
			r = 1;
		else 
			nbfich++;
	}

	// Conditions des valeurs d'entrée
	if (nbfich == 0){
		print_stderr("rm: opérande manquant\n");
		exit(-1);
	}

	// ======================================================================
	// 	 	    PARCOURS DU TAR POUR CHAQUE ARGUMENT
	// ======================================================================

	for (int i=1; i<argc;i++){

		// On evite "-r"
		if(strcmp(argv[i], "-r") != 0) {

			// ----------------------------------------------------------------------
			// 	 	     		TAR/FICHIER/REPERTOIRE A SUPPRIMER
			// ----------------------------------------------------------------------

			//we get the tar to open and the path for the file
			//from tar_and_path
			char ** ar = tar_and_path(argv[i]);

			char * tar = malloc(strlen(ar[0])+sizeof(char));
			strcpy (tar,ar[0]);

			char * arg = NULL;
			if(ar[1] != NULL) {
				arg = malloc(strlen(ar[1])+sizeof(char));
				strcpy (arg,ar[1]);
			}

			// ======================================================================
			// 	 				SUPPRESSION
			// ======================================================================

			// ----------------------------------------------------------------------
			// 	 			SUPPRESION D'UN TAR
			// ----------------------------------------------------------------------

			// Suppresion d'un tar
			if(arg == NULL) {
				if(r)
					unlink(tar);
				else {
					print_stderr("rm: impossible de supprimer '"); 
					print_stderr(argv[i]);
					print_stderr("': est un tar\n");
				}
			}

			// ----------------------------------------------------------------------
			// 	 		  SUPPRESION D'UN FICHIER/REPERTOIRE
			// ----------------------------------------------------------------------

			else {

				// OPENING THE TAR FILE
				int fd = open(tar,O_RDWR);

				if(fd < 0){
					perror("rm: erreur lors de l'ouverture du tar");
					exit(-1);
				}

				struct posix_header * p_hdr;
				char tampon[512];
				int valide = 0;			// 0: fichier ne peut pas etre supprime
										// 1: le fichier peut etre supprime

				char * fich = NULL;		// Nom du fichier
				int rep = 0;			// c'est un repertoire
				unsigned int size;		// Taille du fichier
				off_t longueur = 0;		// Somme de la taille des fichiers avant le repertoire
				off_t supp = 0;			// Somme de la taille du repertoire et de ses fichiers
				off_t dep = 0;			// Somme de la taille des fichiers apres le repertoire

				// ----------------------------------------------------------------------
				// 	 		       PARCOURS DU TAR
				// ----------------------------------------------------------------------

				while(1) {

					// Lecture du bloc

					int rdcount = read(fd,&tampon, BLOCKSIZE);
					if(rdcount<0){
						perror("rm: erreur lors de la lecture du tar");
						close(fd);
						exit(-1);
					}

					// Extraction des informations

					p_hdr = (struct posix_header*)tampon;
					sscanf(p_hdr->size, "%o",&size);

					// On arrive a la fin au bloc nul

					if(strlen(p_hdr-> name) == 0) {
						if (fich == NULL) {
							print_stderr("rm: impossible de supprimer '");
							print_stderr(argv[i]);
							print_stderr("': Aucun fichier ou dossier de ce type\n");
							break;
						}
						else {
							if(rep == 1 && r == 0) {
								print_stderr("rm: impossible de supprimer '"); 
								print_stderr(argv[i]);
								print_stderr("': est un dossier\n");
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


					//	TRAITEMENT LIENS

					int fdlink = open(tar,O_RDONLY);
					if(fdlink<0){

						perror("open");
						exit(-1);
					}

					struct posix_header link;
					unsigned int size_link;


					do{

						// READING AN HEADER

						int rdcount=read(fdlink,&link,BLOCKSIZE);

						//ERROR MANAGMENT

						if(rdcount<0){

							perror("reading tar file");
							close(fdlink);
							exit(-1);
						}

						//READING THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER


						sscanf(link.size, "%o",&size_link);

						if(strcmp(link.linkname,p_hdr->name)==0 && link.typeflag=='1' && size_link==0){

							char* cp [3];
							cp[0]="cp";
							cp[1]=p_hdr->name;
							cp[2]=link.name;
							int ret=tar_vers_tar_mod(cp);							
 
						}  
						

						
				
						//OTHERWISE WE GET TO THE NEXT HEADER
                 
						lseek(fdlink,((size_link+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


					}while(link.name[0]!='\0');

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
				// 	 		  TRAITEMENT DU FICHIER/REPERTOIRE
				// ----------------------------------------------------------------------

				if (valide == 1) {

					// On stocke les donnees a deplacer apres la suppression du repertoire
					// Rappel : le 1er bloc nul a ete lu dans la boucle

					lseek(fd, -dep, SEEK_CUR);
					dep = dep + BLOCKSIZE;
					char mem[dep];

					int rd = read(fd, &mem, dep);
					if(rd<0){
						perror("rm: erreur lors de la lecture du tar");
						close(fd);
						exit(-1);
					}

					// On supprime le fichier/repertoire

					lseek(fd, longueur, SEEK_SET);
					int wr = write(fd, &mem, dep);
					if(wr<0){
						perror("rm: erreur lors de l'écriture du tar");
						close(fd);
						exit(-1);
					}

					ftruncate(fd, longueur+dep);
				}

				close(fd);
			}
		}
	}
	exit(0);
}
