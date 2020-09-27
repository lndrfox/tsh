#include <stdio.h>     // printf
#include <fcntl.h>     // open
#include <stdlib.h>    // exit
#include <unistd.h>    // read close lqeek
#include <sys/types.h> // lseek
#include <string.h>    // strcmp
#include "tar.h"

int main (int argc, char *argv[]) {
	// Deux formats possibles:
	// test.tar
	// -l test.tar

	if(argc < 2 || argc > 3 || (strcmp(argv[1], "-l")!=0 && argc == 3)) {
		printf("Fichier entré invalide\n");
		exit(-1);
	}

	// S'il y a l'option "-l"
	int l = 0;
	if(strcmp(argv[1], "-l")==0 && argc == 3) {
		l = 1;
	} 

	// Ouverture du tar
	int fd;
	if (l == 1) fd = open(argv[2], O_RDONLY);
	else fd = open(argv[1], O_RDONLY);
	if (fd<0) {
		perror("Erreur lors de l'ouverture du tar");
		exit(-1);
	}

	char tampon[512];
	int n;
	unsigned int size;
	struct posix_header * p_hdr;

	while(1) {
		// Lecture du bloc
		n = read(fd, &tampon, 512);
		if(n<0) {
			perror("Erreur lors de la lecture du tar");
			exit(-1);
		}
		if(n == 0) break;

		// Extraction des informations
		p_hdr = (struct posix_header*)tampon;
		if(l == 1) {
			// Faire les cas de "-l"
		}
		if(strlen(p_hdr-> name)!=0)
			printf("%s\n", p_hdr-> name);

		// On passe a l'entete suivante
		sscanf(p_hdr->size,"%o", &size);
		lseek(fd,((size + BLOCKSIZE - 1) >> BLOCKBITS)*512,SEEK_CUR);
	}
	close(fd);
	exit(0);
}
