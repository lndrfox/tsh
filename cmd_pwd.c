#include <stdio.h>  // printf
#include <stdlib.h> // exit
#include <unistd.h> // getcwd
#include <string.h> // strcat
#include <fcntl.h>  //open
#include "tar.h"

int main(int argc, char *argv[]) {
	if(argc != 2) {
		perror("Fichier entré invalide");
		exit(-1);
	}

	// Recupere le chemin du fichier execute
	char* chemin = getcwd(NULL, 0); 
	
	// En attente de la commande cd
	// argv[1] : nom du tar et du fichier (test.tar/nomfichier)
	// Verifier l'existance du fichier

	char* tar = strtok(argv[1], "/");
	char* nomfichier = strtok(NULL, "");
	
	if(nomfichier ==  NULL) {
		printf("Veuillez rentrez un fichier : exemple.tar/fichier\n");
		exit(-1);
	}

	char tampon[512];
	int n;
	struct posix_header * p_hdr;
	int fd = open(tar, O_RDONLY);

	if (fd<0) {
		perror("Erreur lors de l'ouverture du tar");
		exit(-1);
	}
	
	while((n = read(fd, &tampon, 512)) > 0) {
		p_hdr = (struct posix_header*)tampon;
		if(strcmp(p_hdr -> name, nomfichier) == 0) {
			// Affichage du chemin
			printf("Le fichier existe :\n%s/%s/%s \n", chemin, tar, nomfichier); 
			close(fd);
			exit(0);
		}
	}
	
	printf("Fichier inexistant\n");
	close(fd);
	exit(0);
}
