#include <stdio.h>     // printf
#include <fcntl.h>     // open
#include <stdlib.h>    // exit
#include <unistd.h>    // read close lseek
#include <sys/types.h> // lseek
#include <sys/stat.h>  // stat
#include <string.h>    // strcmp strtok strchr
#include <time.h>      // localtime
#include "tar.h"

// Affiche la date
void ptemps(long temps) {
	struct tm * tempsformat = localtime(&temps);
	long tactuel = time(NULL);

	switch (tempsformat->tm_mon) {
		case 0: printf("janv."); break;
		case 1: printf("fevr."); break;
		case 2: printf("mars "); break;
		case 3: printf("avril"); break;
		case 4: printf("mai  "); break;
		case 5: printf("juin "); break;
		case 6: printf("juil."); break;
		case 7: printf("août "); break;
		case 8: printf("sept."); break;
		case 9: printf("oct. "); break;
		case 10: printf("nov. "); break;
		case 11: printf("dec. "); break;

	}
	printf(" %d ", tempsformat -> tm_mday);

	// Si le fichier a ete modifie il y a moins de un an
	if(tactuel-temps < 60*60*24*365) 
		printf("%d:%d ", tempsformat -> tm_hour, tempsformat -> tm_min);
	else
		printf("%d ", tempsformat -> tm_year +1900);
}

void ptype(char t) {
	switch(t) {
			case '0': printf("-"); break;
			case '\0': printf("-"); break;  // A Completer
			case '1': printf("-"); break;	// A Completer
			case '2': printf("l"); break;
			case '3': printf("c"); break;
			case '4': printf("b"); break;	
			case '5': printf("d"); break;
			case '6': printf("p"); break;
			case '7': printf("-"); break;	// A Completer
		}
}

void pdroit(char *d) {
	for(int i = 4; i < 7; i++) {
		switch(d[i]) {
			case '0': printf("---"); break;
			case '1': printf("--x"); break;
			case '2': printf("-w-"); break;
			case '3': printf("-wx"); break;
			case '4': printf("r--"); break;
			case '5': printf("r-x"); break;
			case '6': printf("rw-"); break;
			case '7': printf("rwx"); break;
		}
	}
}

int main (int argc, char *argv[]) {
	// On suppose que la lecture se fait uniquement depuis le tar
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
	int fich = 0;					// 0: C'est un fichier simple
							// 1: C'est un fichier dans un repertoire
							// 2: C'est un repertoire

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

		// (sans la condition ci dessous: affiche des noms d'entetes avec un nom vide)
		if(strlen(p_hdr-> name)!=0) {
			
			// --- Condition de la nature du fichier ---

			// Si c'est un répertoire
			if(p_hdr->typeflag == '5') {// Afficher en couleur bleu sans le '/'
				strtok(p_hdr->name, "/");
				fich = 2;
			}
			// Si c'est un fichier d'un repertoire
			else if(strchr(p_hdr-> name, '/') != NULL) 
				fich = 1;
			// Sinon c'est un fichier simple
			else 
				fich = 0;

			// Si le fichier est dans un repertoire on ne l'affiche pas
			if(fich!=1) {

				// --- S'il y a l'option "-l" ---

				if(l == 1) { 
					// Type de fichier
					ptype(p_hdr-> typeflag);

					// Droits d'accès
					pdroit(p_hdr-> mode);

					// Nombre de liens physiques
					// A terminer avec cd
					
					/*char* chemin = getcwd(NULL, 0);
					struct stat* statbuf;
					stat(chemin, statbuf);
					printf(" %lu ", statbuf -> st_nlink);*/

					// Nom propriétaire
					printf("%s ", p_hdr-> uname);

					// Nom du groupe
					printf("%s ", p_hdr-> gname);

					// Taille en octets
					if(fich == 2)
						printf("4096 ");
					else {
						sscanf(p_hdr->size,"%o", &size);
						printf("%d ", size); 
					}

					// Date de dernière modification 

					// Affiche la date pour les repertoires
					// Les fichier modifies a l'interieur influe sur la date
					// ATTENTION les fichiers de repertoires a l'interieur du repertoire en
					// revanche ne compte pas (i.e repertoire de repertoire de repertoire... etc)
					// LE CODE NE MARCHE PAS :'( HELP
					/*if(fich == 2) {
						long temps;
						long t;
						int taille;

						int nb = 1;
						char* nomrep = malloc(strlen(p_hdr -> name)+1);
						strcpy(nomrep, p_hdr->name);

						// On parcours tous les fichiers appartenant au repertoire et le repertoire
						// (strtok à decoupé le "/" de son nom)
						while(((strchr(p_hdr-> name, '/') != NULL) || nb == 1) && n > 0 && strlen(p_hdr-> name) != 0) {
							sscanf(p_hdr -> mtime,"%lo", &t);

							// On prend le temps le plus récent
							if(temps < t) {
								temps = t;
							}
							nb ++;

							sscanf(p_hdr->size,"%o", &taille);
							lseek(fd,((taille + BLOCKSIZE - 1) >> BLOCKBITS)*512,SEEK_CUR);
							n = read(fd, &tampon, 512);
							p_hdr = (struct posix_header*)tampon;
						}

						// On retourne au repertoire
						while(strcmp(p_hdr -> name, nomrep) != 0 && n > 0 && strlen(p_hdr-> name) != 0) {
							lseek(fd,-(((taille + BLOCKSIZE - 1) >> BLOCKBITS)*512),SEEK_CUR);
							n = read(fd, &tampon, 512);
							p_hdr = (struct posix_header*)tampon;
							sscanf(p_hdr->size,"%o", &taille);
						}
						ptemps(temps);
					}
					
					else {*/
						long temps;
						sscanf(p_hdr-> mtime,"%lo", &temps);
						ptemps(temps);
					//}

				}
				// Nom du fichier
				printf("%s\n", p_hdr-> name);
			}
		}

		// On passe a l'entete suivante
		sscanf(p_hdr->size,"%o", &size);
		lseek(fd,((size + BLOCKSIZE - 1) >> BLOCKBITS)*512,SEEK_CUR);
	}
	close(fd);
	exit(0);
}
