#include <stdio.h>		// sscanf sprintf perror
#include <sys/types.h>	// lseek
#include <fcntl.h>		// open
#include <unistd.h>		// read close lseek write
#include <string.h>		// strcat strcmp
#include <stdlib.h>		// exit
#include "tar.h"
#include "print.h"


// Format: ./mkdir fichiertar.tar directory otherdirectory ...

int main(int argc, char *argv[]){

	// Conditions des valeurs d'entrée
	if (argc < 2){
		prints("\033[1;31mEntrée invalide \n");
		exit(-1);
	}

	// Ouverture du tar
	int fd = open(argv[1],O_RDWR);
	if(fd < 0){
		perror("\033[1;31mErreur lors de l'ouverture du tar\n");
		exit(-1);
	}

	struct posix_header * p_hdr;
	char tampon[512];

	// On parcours pour chaque repertoire
	for (int i=2; i<argc;i++){

		char* rep = NULL;
		off_t longueur = 0;
		size_t supp = 0;
		size_t dep = 0;

		// On parcours le tar
		while(1) {

			// Lecture du bloc
			int rdcount = read(fd,&tampon, BLOCKSIZE);
			if(rdcount<0){
				perror("\033[1;31mErreur lors de la lecture du tar\n");
				close(fd);
				exit(-1);
			}
			// Extraction des informations
			p_hdr = (struct posix_header*)tampon;
			unsigned int size;
			sscanf(p_hdr->size, "%o",&size);

			// On arrive a la fin au bloc nul
			if(strlen(p_hdr-> name) == 0) {
				if (rep == NULL) {
					prints("\033[1;31mLe répertoire n'existe pas\n");
					exit(-1);
				}
				else {
					dep = dep + 512;
					break;
				}
			}

			// Si on trouve le repertoire
			if(strcmp(p_hdr->name, argv[i]) == 0 && p_hdr->typeflag == '5') {
				rep = malloc(strlen(p_hdr->name) + 1);
				strcpy(rep, strtok(p_hdr->name,"/"));
			}

			// Stockage des octets a utiliser lors de la suppresion
			if(rep != NULL) {
				// Le repertoire et ses sous fichiers a supprimer
				strtok(p_hdr-> name, "/");
				if(strcmp(p_hdr-> name, rep) == 0) 
					supp = supp + 512 + size;

				// Toutes donnees se situant apres le repertoire
				else
					dep = dep + 512 + size; 
			}
			// Toutes donnees avant d'avoir trouve le repertoire
			else 
				longueur = longueur + 512 + size;

			// On passe a l'entete suivante
			lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);

		}

		// Traitement du repertoire
		// (Le 1er bloc nul a ete lu dans la boucle)

		// On stocke les donnees a deplacer apres la suppression du repertoire

		lseek(fd, -dep, SEEK_CUR);
		dep = dep + 512;
		char mem[dep];

		printd(longueur); prints(" : "); printd(supp); prints(" : "); printd(dep); prints("\n");

		int rd = read(fd, &mem, dep);
		if(rd<0){
			perror("\033[1;31mErreur lors de la lecture du tar");
			close(fd);
			exit(-1);
		}

		// On supprime le repertoire

		lseek(fd, longueur, SEEK_SET);
		int wr = write(fd, &mem, dep);
		if(wr<0){
			perror("\033[1;31mErreur lors de l'écriture du tar");
			close(fd);
			exit(-1);
		}

		ftruncate(fd, longueur+dep);

		// Retour au depart

		lseek(fd,0,SEEK_SET);

	}

	close(fd);
	exit(0);
}