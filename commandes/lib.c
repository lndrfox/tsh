#include <stdio.h>     // sscanf sprintf
#include <fcntl.h>     // open
#include <stdlib.h>    // exit getenv
#include <unistd.h>    // read close lseek
#include <sys/types.h> // lseek
#include <string.h>    // strcmp
#include <time.h>      // localtime time
#include "tar.h"
#include "print.h"
#include "lib.h"

// ----------------------------------------------------------------------
// 	 	     	        UTILITAIRE
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
// 	 	     	      AFFICHAGE TAR
// ----------------------------------------------------------------------

typedef struct affichage {
	char * affichage;
	int size;
} a;

void init(a * a, int size) {
	a -> affichage = malloc(size + 1);
	a -> size = size;
}

void etendre(a * a, int size) {
	a -> affichage = realloc(a -> affichage, (a -> size) + size + 1);
	a -> size = (a -> size) + size;
}

void ajout(a * a, char * ajout) {
	strcat(a -> affichage, ajout);
}

char * afficher(a * a) {
	return a -> affichage;
}

int taille(a * a) {
	return a -> size;
}

// Affiche la date
void ptemps(a * a, long temps) {
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
void ptype(a * a, char t) {
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
void pdroit(a * a, char *d) {
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
void plink(a * a, char * tar, struct posix_header * p_hdr1) {

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
void afficheNom(a * a,struct posix_header * p_hdr, char* rep, int repexiste, int option) {

	// Si c'est un repertoire: couleur bleu
	if(p_hdr -> typeflag == '5') 
		strcat(a -> affichage, "\033[1;34m");
	// Si c'est un lien symbolique couleur cyan
	if(p_hdr -> typeflag == '2')
		strcat(a -> affichage, "\033[1;36m");
	// Si c'est un tube couleur jaune
	if(p_hdr -> typeflag == '6')
		strcat(a -> affichage, "\033[0;33m\033[48;5;236m");

	// Si c'est un fichier du repertoire choisi on affiche que le nom apres rep/...
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
	if(p_hdr -> typeflag == '2' && option == 1) {
		strcat(a -> affichage, " -> ");
		strcat(a -> affichage, p_hdr -> linkname);
	}

	// On ajoute un retour a la ligne ou un tab

	strcat(a -> affichage, "\n");
}

// Nombre de blocks occupé par le tar ou le repertoire a afficher
int total(char * tar, char * rep) {

	int fd = open(tar, O_RDONLY);
	struct posix_header* p_hdr;
	char * tampon[512];
	unsigned int size;
	int rd;
	int total = 0;

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
