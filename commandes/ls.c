#include <stdio.h>     // sscanf sprintf perror
#include <fcntl.h>     // open
#include <stdlib.h>    // exit
#include <unistd.h>    // read close lseek write
#include <sys/types.h> // lseek
#include <string.h>    // strcmp strtok strchr
#include <time.h>      // localtime time
#include "tar.h"
#include "print.h"

// Affiche la date
void ptemps(long temps) {
	struct tm * tempsformat = localtime(&temps);
	long tactuel = time(NULL);

	switch (tempsformat->tm_mon) {
		case 0: prints("janv."); break;
		case 1: prints("fevr."); break;
		case 2: prints("mars "); break;
		case 3: prints("avril"); break;
		case 4: prints("mai  "); break;
		case 5: prints("juin "); break;
		case 6: prints("juil."); break;
		case 7: prints("août "); break;
		case 8: prints("sept."); break;
		case 9: prints("oct. "); break;
		case 10: prints("nov. "); break;
		case 11: prints("dec. "); break;

	}
	prints(" ");
	printd(tempsformat -> tm_mday);
	prints(" ");

	// Si le fichier a ete modifie il y a moins de un an
	if(tactuel-temps < 60*60*24*365) {
		printd(tempsformat -> tm_hour);
		prints(":");
		printd(tempsformat -> tm_min);
	}
	else
		printd(tempsformat -> tm_year +1900);
		prints(" ");
}

// Affiche le type du fichier
void ptype(char t) {
	switch(t) {
			case '0': prints("-"); break;
			case '\0': prints("-"); break;
			case '1': prints("l"); break;
			case '2': prints("l"); break;
			case '3': prints("c"); break;
			case '4': prints("b"); break;	
			case '5': prints("d"); break;
			case '6': prints("p"); break;
			case '7': prints("s"); break;
		}
}

// Affiche les droits d'acces du fichier
void pdroit(char *d) {
	for(int i = 4; i < 7; i++) {
		switch(d[i]) {
			case '0': prints("---"); break;
			case '1': prints("--x"); break;
			case '2': prints("-w-"); break;
			case '3': prints("-wx"); break;
			case '4': prints("r--"); break;
			case '5': prints("r-x"); break;
			case '6': prints("rw-"); break;
			case '7': prints("rwx"); break;
		}
	}
}

int main (int argc, char *argv[]) {
	// On suppose que la lecture se fait uniquement depuis le tar
	// Quatre formats possibles:
	// test.tar
	// test.tar/rep
	// -l test.tar
	// -l test.tar/rep

	// Conditions des valeurs d'entrée
	if(argc < 2 || argc > 3 || (strcmp(argv[1], "-l")!=0 && argc == 3)) {
		prints("\033[1;31mEntrée invalide\n");
		exit(-1);
	}

	// S'il y a l'option "-l"
	int l = 0;
	if(strcmp(argv[1], "-l")==0 && argc == 3) {
		l = 1;
	} 

	// ======================================================================
	// 			      OUVERTURE DU TAR
	// ======================================================================

	int fd;
	char* rep = NULL;
	int argnum;

	if (l == 1)	
		argnum = 2;
	else 
		argnum = 1;
	
	// Si on veut afficher un repertoire du tar
	if(strchr(argv[argnum], '/') != NULL) {
		strtok(argv[argnum], "/");		// Le nom du tar
		rep = strtok(NULL, "");	 		// Le nom du repertoire
	}

	fd = open(argv[argnum], O_RDONLY);
	if (fd<0) {
		perror("\033[1;31mErreur lors de l'ouverture du tar");
		exit(-1);
	}

	// ======================================================================
	// 	 			LECTURE DU TAR
	// ======================================================================

	char tampon[512];
	int n;
	unsigned int size;
	struct posix_header * p_hdr;
	int fich = 0;					// 0: C'est un fichier simple
							// 1: C'est un fichier dans un repertoire
							// 2: C'est un repertoire

	int repexiste = 0;				// 0: Le repertoire correspondant a rep n'a pas ete trouve
							// 1: Le repertoire a ete trouve

	while(1) {
		// Lecture du bloc
		n = read(fd, &tampon, 512);
		if(n<0) {
			perror("\033[1;31mErreur lors de la lecture du tar");
			exit(-1);
		}
		if(n == 0) break;

		// Extraction des informations
		p_hdr = (struct posix_header*)tampon;

		// On evite les deux blocs finaux formés uniquement de zéros
		if(strlen(p_hdr-> name)!=0) {
			
			// ----------------------------------------------------------------------
			// 	 	     CONDITION DE LA NATURE DU FICHIER
			// ----------------------------------------------------------------------

			// Si c'est un répertoire
			if(p_hdr->typeflag == '5') {
				strtok(p_hdr->name, "/");
				if(rep != NULL) {
					if (strcmp(p_hdr -> name, rep) == 0)
						repexiste = 1;
				}
				fich = 2;
			}
			// Si c'est un fichier d'un repertoire
			else if(strchr(p_hdr-> name, '/') != NULL) 
				fich = 1;
			// Sinon c'est un fichier simple
			else 
				fich = 0;

			// ----------------------------------------------------------------------
			// 	 		AFFICHAGE DES INFORMATIONS
			// ----------------------------------------------------------------------

			// On stock un mot de la taille de rep avec les strlen(rep) 1ere lettres du nom du fichier
			char* isrep; 
			if(rep != NULL && repexiste == 1) {
				isrep = malloc(strlen(rep) +1);
				strncat(isrep, p_hdr-> name, strlen(rep));
			}
		
			// Format test.tar: Si le fichier est dans un repertoire on ne l'affiche pas
			// Format test.tar/rep: Sinon on affiche uniquement ses fichiers
			if((rep == NULL && fich!=1) || (rep != NULL && repexiste == 1 && fich == 1 && strcmp(isrep, rep) == 0)) {

				// S'il y a l'option "-l"
				if(l == 1) { 
					// Type de fichier
					ptype(p_hdr-> typeflag);

					// Droits d'accès
					pdroit(p_hdr-> mode);

					// Nom propriétaire
					prints(" ");
					prints(p_hdr-> uname);
					prints(" ");

					// Nom du groupe
					prints(p_hdr-> gname);
					prints(" ");

					// Taille en octets
					sscanf(p_hdr->size,"%o", &size);
					printd(size);
					prints(" ");

					// Date de dernière modification 
					long temps;
					sscanf(p_hdr-> mtime,"%lo", &temps);
					ptemps(temps);
				}
				// Nom du fichier

				// Si c'est un repertoire: couleur bleu
				if(fich == 2) 
					prints("\033[1;34m");

				// Si c'est un fichier du repertoire choisi on affiche que le nom apres rep/
				if(rep != NULL && repexiste == 1) {
					strtok(p_hdr-> name, "/");
					char* sousfichier = strtok(NULL, "");
					prints(sousfichier);
					prints("\n");
				}
				else {
					prints(p_hdr-> name);
					prints("\n");
				}

				// Renitialise la couleur
				if(fich == 2)
					prints("\033[0m"); 

			}
			
			if (rep != NULL && repexiste == 1) 
				free(isrep);
		}

		// On passe a l'entete suivante
		sscanf(p_hdr->size,"%o", &size);
		lseek(fd,((size + BLOCKSIZE - 1) >> BLOCKBITS)*512,SEEK_CUR);
	}

	// Si on ne rencontre pas le repertoire recherche
	if(rep!= NULL && repexiste == 0) 
		prints("\033[1;31mLe répertoire n'existe pas\n");

	close(fd);
	exit(0);
}
