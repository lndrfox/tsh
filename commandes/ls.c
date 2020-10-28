#include <stdio.h>     // sscanf sprintf perror
#include <fcntl.h>     // open
#include <stdlib.h>    // exit getenv
#include <unistd.h>    // read close lseek write
#include <sys/types.h> // lseek
#include <string.h>    // strcmp strtok strchr
#include <time.h>      // localtime time
#include "tar.h"
#include "tar_nav.h"
#include "cd.h"
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
	else{
		printd(tempsformat -> tm_year +1900);
	}
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

// Affiche le nom du fichier
void afficheNom(struct posix_header * p_hdr, char* rep, int repexiste) {

	// Si c'est un repertoire: couleur bleu
	if(p_hdr -> typeflag == '5') 
		prints("\033[1;34m");

	// Si c'est un fichier du repertoire choisi on affiche que le nom apres rep1/rep2/...
	if(rep != NULL && repexiste == 1) {

		char * nom = malloc(strlen(p_hdr ->name) - strlen(rep) + 1);
		strcpy(nom, &p_hdr -> name[strlen(rep)]);
		printsss("", nom, "\n");
	}

	// Sinon on affiche son nom complet
	else
		printsss("", p_hdr-> name, "\n");

	// Renitialise la couleur
	if(p_hdr -> typeflag == '5')
		prints("\033[0m"); 
}

// Retourne la profondeur d'un fichier
int profondeur (struct posix_header * p_hdr) {
	int profondeur = -1;

	char * nom = malloc(strlen(p_hdr->name)+1);
	strcpy(nom, p_hdr -> name);
	char *buf = strtok(nom, "/");

	while(buf != NULL) {
		profondeur ++;
		buf = strtok(NULL, "/");
	}

	return profondeur;   
}

// Retourne si le fichier appartient au repertoire
int estDansRep(char * name, char * rep) {
	char * repertoire = malloc(strlen(rep) +1);
	char * isrep = malloc(strlen(rep) +1);
	strncpy(isrep, name, strlen(rep));
	strncpy(repertoire, rep, strlen(rep));

	if(strcmp(isrep, repertoire) == 0)
		return 1;
	else
		return 0;
}

int main (int argc, char *argv[]) {
	// On suppose que la lecture se fait uniquement depuis le tar
	// Quatre formats possibles:
	// (1) test.tar
	// (2) test.tar/rep1/rep2/...
	// (3) -l test.tar
	// (4) -l test.tar/rep1/rep2/...

	// Conditions des valeurs d'entrée
	if(argc != 2 && (argc != 3 || strcmp(argv[1], "-l") != 0)) {
		prints("\033[1;31mEntrée invalide\n");
		exit(-1);
	}

	// S'il y a l'option "-l"
	int l = 1;
	if(strcmp(argv[1], "-l") == 0 && argc == 3)
		l = 2;
	

	// ======================================================================
	// 			      OUVERTURE DU TAR
	// ======================================================================

	int fd;
	char* rep = NULL;
	
	// Si on veut afficher un repertoire du tar
	if(strchr(argv[l], '/') != NULL) {
		strtok(argv[l], "/");		// Le nom du tar
		rep = strtok(NULL, "");		// Le nom du repertoire
	}

	fd = open(argv[l], O_RDONLY);
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

	int p;				// profondeur

	int repexiste = 0;		// 0: Le repertoire correspondant a rep n'a pas ete trouve
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
				// Si le repertoire correspond au repertoire a afficher
				if(rep != NULL) {
					if (strcmp(p_hdr -> name, rep) == 0) {
						repexiste = 1;
						p = profondeur(p_hdr) + 1;
					}
				}
			}

			// ----------------------------------------------------------------------
			// 	 		AFFICHAGE DES INFORMATIONS
			// ----------------------------------------------------------------------

			// Format test.tar: Si le fichier est dans un repertoire on ne l'affiche pas
			// Format test.tar/rep1/rep2/...: Sinon on affiche uniquement ses fichiers
			if((rep == NULL && profondeur(p_hdr) == 0) || (rep != NULL && repexiste == 1 && profondeur(p_hdr) == p && estDansRep(p_hdr -> name, rep) == 1)) {

				// S'il y a l'option "-l"
				if(l == 2) { 
					// Type de fichier
					ptype(p_hdr-> typeflag);

					// Droits d'accès
					pdroit(p_hdr-> mode);

					// Nom propriétaire
					printsss(" ", p_hdr-> uname, " ");

					// Nom du groupe
					printsss("", p_hdr-> gname, " ");

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
				afficheNom(p_hdr, rep, repexiste);

			}
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
