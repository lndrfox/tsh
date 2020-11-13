#include <stdio.h>     // sscanf sprintf perror
#include <fcntl.h>     // open
#include <stdlib.h>    // exit getenv
#include <unistd.h>    // read close lseek write
#include <sys/types.h> // lseek
#include <string.h>    // strcmp strtok strchr
#include <time.h>      // localtime time
#include "tar.h"
#include "print.h"

// ----------------------------------------------------------------------
// 	 	     		UTILITAIRE
// ----------------------------------------------------------------------

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

// ----------------------------------------------------------------------
// 	 	     		AFFICHAGE
// ----------------------------------------------------------------------

typedef struct affichage {
	char * affichage;
	int size;
} affichage;

void init(affichage * a, int size) {
	a -> affichage = malloc(size + 1);
	a -> size = size;
}

void etendre(affichage * a, int size) {
	a -> affichage = realloc(a -> affichage, size + 1);
	a -> size = size;
}

// Affiche la date
void ptemps(affichage * a, long temps) {
	struct tm * tempsformat = localtime(&temps);
	long tactuel = time(NULL);

	// Mois
	switch (tempsformat->tm_mon) {
		case 0: strcat(a -> affichage, "janv."); break;
		case 1: strcat(a -> affichage, "fevr."); break;
		case 2: strcat(a -> affichage, "mars "); break;
		case 3: strcat(a -> affichage, "avril"); break;
		case 4: strcat(a -> affichage, "mai  "); break;
		case 5: strcat(a -> affichage, "juin "); break;
		case 6: strcat(a -> affichage, "juil."); break;
		case 7: strcat(a -> affichage, "août "); break;
		case 8: strcat(a -> affichage, "sept."); break;
		case 9: strcat(a -> affichage, "oct. "); break;
		case 10:strcat(a -> affichage, "nov. "); break;
		case 11:strcat(a -> affichage, "dec. "); break;

	}

	// Jour
	char jour[4];
	sprintf(jour, " %d ", tempsformat -> tm_mday);
	strcat(a -> affichage, jour);

	// Si le fichier a ete modifie il y a moins de 6 mois
	if(tactuel-temps < 60*60*24*183) {
		char time[5];
		sprintf(time, "%d:%d", tempsformat -> tm_hour, tempsformat -> tm_min);
		strcat(a -> affichage, time);
	}
	else{
		char annee[4];
		sprintf(annee, "%d", tempsformat -> tm_year + 1900);
		strcat(a -> affichage, annee);
	}
	strcat(a -> affichage, " ");
}

// Affiche le type du fichier
void ptype(affichage * a, char t) {
	switch(t) {
			case '0':  strcat(a -> affichage, "-"); break;
			case '\0': strcat(a -> affichage, "-"); break;
			case '1':  strcat(a -> affichage, "-"); break;
			case '2':  strcat(a -> affichage, "l"); break;
			case '3':  strcat(a -> affichage, "c"); break;
			case '4':  strcat(a -> affichage, "b"); break;	
			case '5':  strcat(a -> affichage, "d"); break;
			case '6':  strcat(a -> affichage, "p"); break;
			case '7':  strcat(a -> affichage, "s"); break;
		}
}

// Affiche les droits d'acces du fichier
void pdroit(affichage * a, char *d) {
	for(int i = 4; i < 7; i++) {
		switch(d[i]) {
			case '0': strcat(a -> affichage, "---"); break;
			case '1': strcat(a -> affichage, "--x"); break;
			case '2': strcat(a -> affichage, "-w-"); break;
			case '3': strcat(a -> affichage, "-wx"); break;
			case '4': strcat(a -> affichage, "r--"); break;
			case '5': strcat(a -> affichage, "r-x"); break;
			case '6': strcat(a -> affichage, "rw-"); break;
			case '7': strcat(a -> affichage, "rwx"); break;
		}
	}
}

// Affiche le nombre de liens physiques du fichier
void plink(affichage * a, char * tar, struct posix_header * p_hdr1) {

	int fd = open(tar, O_RDONLY);
	int rd;
	char tampon[512];	
	struct posix_header * p_hdr2;
	unsigned int size;

	char *file =   p_hdr1 -> name;
	char *file_l = p_hdr1 -> linkname;;
	char *f =  "";
	char *fl = "";
	int compt = 1;


	while(1) {

		// Lecture du fichier tar
	    if((rd = read(fd,tampon, BLOCKSIZE)) <0) {
			perror("\033[1;31mErreur lors de la lecture du tar\033[0m");
			exit(-1);
		}
		if(rd == 0) break;

		// Extraction des donnees
	    p_hdr2 = (struct posix_header*)tampon;

	    f =  p_hdr2 -> name;
	    fl = p_hdr2 -> linkname;

	    // Comparaison entre le fichier mis en parametre et tous les autres fichiers
	 	if(strlen(fl) > 0 && p_hdr2 -> typeflag != '2') {
			if((strcmp(file, f)==0) || (strcmp(file, fl)==0) || (strcmp(file_l, f)==0) || (strcmp(file_l, fl)==0))
				compt ++;
		}
		
		// On passe a l'entête suivante
		sscanf(p_hdr2->size,"%o", &size);
	    lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);
	 }

	close(fd);
	char hardlink[4];
	sprintf(hardlink, " %d ", compt);
	strcat(a -> affichage, hardlink);
}

// Affiche le nom du fichier
void afficheNom(affichage * a,struct posix_header * p_hdr, char* rep, int repexiste, int option) {

	// Si c'est un repertoire: couleur bleu
	if(p_hdr -> typeflag == '5') 
		strcat(a -> affichage, "\033[1;34m");
	// Si c'est un lien symbolique couleur cyan
	if(p_hdr -> typeflag == '2')
		strcat(a -> affichage, "\033[1;36m");
	// Si c'est un tube couleur jaune
	if(p_hdr -> typeflag == '6')
		strcat(a -> affichage, "\033[0;33m\033[48;5;236m");

	// Si c'est un fichier du repertoire choisi on affiche que le nom apres rep1/rep2/...
	if(rep != NULL && repexiste == 1) {
		char * nom = malloc(strlen(p_hdr ->name) - strlen(rep) + 1);
		strcpy(nom, &p_hdr -> name[strlen(rep)]);
		strcat(a -> affichage, nom);
	}

	// Sinon on affiche son nom complet
	else
		strcat(a -> affichage, p_hdr-> name);

	// Renitialise la couleur
	strcat(a -> affichage, "\033[0m");

	// Si c'est un symbolique et qu'il y a l'option "-l" on affiche le fichier sur lequel il pointe 
	if(p_hdr -> typeflag == '2' && option == 2 ) {
		strcat(a -> affichage, " -> ");
		strcat(a -> affichage, p_hdr -> linkname);
	}

	strcat(a -> affichage, "\n");
}

// Nombre de blocks occupé par le tar ou le repertoire a afficher
int total(char * tar, char * rep) {

	int fd = open(tar, O_RDONLY);
	struct posix_header* p_hdr;
	char * tampon[512];
	unsigned int size;
	int rd;
	int total;

	while(1) {

		// Lecture du fichier tar
	    if((rd = read(fd,tampon, BLOCKSIZE)) <0) {
			perror("\033[1;31mErreur lors de la lecture du tar\033[0m");
			exit(-1);
		}
 
		// Extraction des informations
		p_hdr = (struct posix_header*)tampon;

		// Le premier block final est lu
		if(strlen(p_hdr-> name) == 0) break;

		// Taille du fichier
		sscanf(p_hdr->size,"%o", &size);

		if((rep != NULL && estDansRep(p_hdr -> name, rep) == 1) || rep == NULL)
				total += 1 + ((size + BLOCKSIZE - 1) >> BLOCKBITS);

		// On passe a l'entete suivante
		lseek(fd,((size + BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);
	}

	// On ajoute les deux blocs finaux si ce n'est pas la taille 
	// occupe par un repertoire qui doit etre affiche
	if(rep == NULL)
		total += 2;

	close(fd);
	return total;

}

int main (int argc, char *argv[]) {
	// On suppose que la lecture se fait uniquement depuis le tar
	// Quatre formats possibles:
	// (1) ls
	// (2) ls rep1/rep2/...
	// (3) ls -l
	// (4) ls -l /rep1/rep2/...

	char * tar = getenv("tar");

	// Conditions des valeurs d'entrée
	if(argc != 1 && argc != 2 && (argc != 3 || strcmp(argv[1], "-l") != 0)) {
		prints("\033[1;31mEntrée invalide\n\033[0m");
		exit(-1);
	}

	// S'il y a l'option "-l"
	int l = 1;
	if(argc > 1) {
		if(strcmp(argv[1], "-l") == 0)
			l = 2;
	}
	
	// ======================================================================
	// 			    OUVERTURE DU TAR
	// ======================================================================

	int fd;
	char* rep = NULL;
	
	// Si on veut afficher un repertoire du tar
	if((argc == 2 && l == 1) || (argc == 3 && l == 2))
		rep = argv[l];

	fd = open(tar, O_RDONLY);
	if (fd<0) {
		perror("\033[1;31mErreur lors de l'ouverture du tar\033[0m");
		exit(-1);
	}

	// ======================================================================
	// 	 		     LECTURE DU TAR
	// ======================================================================

	char tampon[512];				// lecture du bloc stocke ici
	int n;							// nombre de caractres lus dans un tampon
	unsigned int size;				// taille du fichier lu
	struct posix_header * p_hdr;	// header du fichier
	int p;							// profondeur

	int repexiste = 0;				// 0: Le repertoire correspondant a rep n'a pas ete trouve
							// 1: Le repertoire a ete trouve

	affichage * a = malloc(sizeof(affichage));	// affichage a la fin du traitement
	init(a, 100);

	while(1) {
		// Lecture du bloc
		n = read(fd, &tampon, BLOCKSIZE);
		if(n<0) {
			perror("\033[1;31mErreur lors de la lecture du tar\033[0m");
			exit(-1);
		}
		if(n == 0) break;

		// Extraction des informations
		p_hdr = (struct posix_header*)tampon;

		// On evite les deux blocs finaux formés uniquement de zéros
		if(strlen(p_hdr-> name)!=0) {
			
			// ----------------------------------------------------------------------
			// 	 	     	  REPERTOIRE A AFFICHER
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

				// On realloue de la memoire afin que l'affichage puisse tout contenir
				etendre (a, a -> size + 100);

				// S'il y a l'option "-l"
				if(l == 2) { 
					// Type de fichier
					ptype(a, p_hdr-> typeflag);

					// Droits d'accès
					pdroit(a, p_hdr-> mode);

					// Liens physiques
					plink(a, tar, p_hdr);

					// Nom propriétaire
					char proprietaire[strlen(p_hdr-> uname)+2];
					sprintf(proprietaire, " %s ", p_hdr-> uname);
					strcat(a -> affichage, proprietaire);

					// Nom du groupe
					char groupe[strlen(p_hdr-> gname)+1];
					sprintf(groupe, "%s ", p_hdr-> gname);
					strcat(a -> affichage, groupe);

					// Taille en octets
					sscanf(p_hdr->size,"%o", &size);
					char taille[40];
					sprintf(taille, "%d ", size);
					strcat(a -> affichage, taille);

					// Date de dernière modification 
					long temps;
					sscanf(p_hdr-> mtime,"%lo", &temps);
					ptemps(a, temps);
				}
				// Nom du fichier
				afficheNom(a, p_hdr, rep, repexiste, l);

			}
		}

		// On passe a l'entete suivante
		sscanf(p_hdr->size,"%o", &size);
		lseek(fd,((size + BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);
	}

	// Si on ne rencontre pas le repertoire recherche
	if(rep!= NULL && repexiste == 0) 
		printsss("ls: impossible d'accéder à '", rep ,"': Aucun fichier ou dossier de ce type\n");

	printd(total(tar, rep));
	prints(a -> affichage);

	close(fd);
	exit(0);
}
