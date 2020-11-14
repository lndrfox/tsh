#include <stdio.h>		// sscanf perror
#include <sys/types.h>		// lseek ftruncate
#include <fcntl.h>		// open
#include <unistd.h>		// read close lseek write ftruncate
#include <string.h>		// strcat strcmp
#include <stdlib.h>		// exit
#include "tar.h"
#include "print.h"
#include "lib.h"


// Format: ./rmdir fichiertar.tar directory otherdirectory ...
// (Un repertoire doit avoir '/' a la fin)

int main(int argc, char *argv[]){

	// ======================================================================
	// 	 			INITIALISATION
	// ======================================================================

	char * tar = getenv("tar");

	// Conditions des valeurs d'entrée
	if (argc < 2){
		prints("rmdir: opérande manquant\n");
		exit(-1);
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
	// 	 	    PARCOURS DU TAR POUR CHAQUE REPERTOIRE
	// ======================================================================

	for (int i=1; i<argc;i++){

		int valide = 0;			// 0: le repertoire ne peut pas etre supprime
						// 1: le repertoire peut etre supprime

		char* rep = NULL;		// Nom du repertoire
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
				perror("\033[1;31mErreur lors de la lecture du tar\033[0m");
				close(fd);
				exit(-1);
			}

			// Extraction des informations

			p_hdr = (struct posix_header*)tampon;
			sscanf(p_hdr->size, "%o",&size);

			// On arrive a la fin au bloc nul

			if(strlen(p_hdr-> name) == 0) {
				if (rep == NULL) {
					printsss("rmdir: impossible de supprimer '", argv[i], "': Aucun fichier ou dossier de ce type\n");
					break;
				}
				else {
					if(supp != BLOCKSIZE) {
						printsss("rmdir: impossible de supprimer '", argv[i], "': Le dossier n'est pas vide\n");
						break;
					}
					else {
						valide = 1;
						dep = dep + BLOCKSIZE;
						break;
					}
				}
			}

			// Si on trouve le repertoire

			if(strcmp(p_hdr -> name, argv[i]) == 0 && p_hdr->typeflag == '5') {
				rep = malloc(strlen(p_hdr->name) + 1);
				strcpy(rep, p_hdr->name);
			}

			// Stockage des octets a utiliser lors de la suppresion

			if(rep != NULL) {
				// Le repertoire et ses fichiers a supprimer
				if(estDansRep(p_hdr-> name, rep) == 1) 
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
