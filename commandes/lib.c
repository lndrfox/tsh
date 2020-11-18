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

int existant (char * tar, char * file, char * rep) {

	int fd = open(tar, O_RDONLY);
	struct posix_header* p_hdr;
	char * tampon[512];
	unsigned int size;
	int rd;

	// Format du nom
	char * f;

	if(rep != NULL) {
		f = malloc(strlen(file) + strlen(rep) + 1);
		strcpy(f, rep);
		strcat(f, file);
	}
	else {
		f = malloc(strlen(file) + 1);
		strcpy(f, file);
	}


	while(1) {
	    if((rd = read(fd,tampon, BLOCKSIZE)) < 0) {
			perror("\033[1;31mErreur lors de la lecture du tar\033[0m");
			exit(-1);
		}
		if (rd == 0) break;

		p_hdr = (struct posix_header*)tampon;

		if(strcmp(p_hdr-> name, f) == 0) return 1;

		// On passe a l'entete suivante
		sscanf(p_hdr->size,"%o", &size);
		lseek(fd,((size + BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);
	}

	close(fd);
	return 0;
}

char * dernier_existant (char * tar, char * var_rep, int argc, char *argv[]) {

	int fd = open(tar, O_RDONLY);
	struct posix_header* p_hdr;
	char * tampon[512];
	unsigned int size;
	int rd;
	char * rep;
	char * dernier_rep = "";

	for(int i = 1; i < argc; i++) { 

		lseek(fd,0 ,SEEK_SET);

		if(strcmp(argv[i], "-l") != 0) {

			char * arg;
			// Si un repertoire est entre sans '/'
			if(argv[i][strlen(argv[i]) - 1] != '/') {
				arg = malloc(strlen(argv[i]) + 2);
				strcpy(arg, argv[i]);
				strcat(arg, "/");
			}
			else 
				arg = argv[i];

			if (var_rep != NULL) {
				char * nouv_rep = malloc(strlen(var_rep) + strlen(arg) + 1);
				strcpy(nouv_rep, var_rep);
				strcat(nouv_rep, arg);
				rep = nouv_rep;
			}
			else
				rep = arg;

			while(1) {
			    if((rd = read(fd,tampon, BLOCKSIZE)) < 0) {
					perror("\033[1;31mErreur lors de la lecture du tar\033[0m");
					exit(-1);
				}
				if (rd == 0) break;

				p_hdr = (struct posix_header*)tampon;

				if(strlen(p_hdr-> name) != 0 && strcmp(p_hdr-> name, rep) == 0) {
					if(var_rep == NULL || (var_rep != NULL && estDansRep(rep, var_rep) == 1))
						dernier_rep = argv[i];
				}

				// On passe a l'entete suivante
				sscanf(p_hdr->size,"%o", &size);
				lseek(fd,((size + BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);
			}
		}
	}
	
	close(fd);
	return dernier_rep;
}

// ----------------------------------------------------------------------
// 	 	     		AFFICHAGE TAR
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
			case '2':  strcat(a -> affichage, "l"); break;
			case '3':  strcat(a -> affichage, "c"); break;
			case '4':  strcat(a -> affichage, "b"); break;	
			case '5':  strcat(a -> affichage, "d"); break;
			case '6':  strcat(a -> affichage, "p"); break;
			default :  strcat(a -> affichage, "-"); break;
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
void afficheNom(a * a, struct posix_header * p_hdr, char* rep, int option, char * tar) {

	char * name = p_hdr-> name;
	char n[strlen(p_hdr-> name)];

	// Si c'est un repertoire: couleur bleu
	if(p_hdr -> typeflag == '5'){
		strcat(a -> affichage, "\033[1;34m");
		// Supprimer '/' du nom
		strcpy(n, p_hdr-> name);
		n[strlen(n) - 1] = '\0';
		name = n;
	}
	// Si c'est un lien symbolique couleur cyan/rouge
	else if(p_hdr -> typeflag == '2') {
		if(existant(tar, p_hdr -> linkname, rep) == 1)
			strcat(a -> affichage, "\033[1;36m");
		else
			strcat(a -> affichage, "\033[1;31m\033[48;5;236m");
	}
	// Si c'est un tube couleur jaune
	else if(p_hdr -> typeflag == '6')
		strcat(a -> affichage, "\033[0;33m\033[48;5;236m");
	// Si c'est un fichier spécial caractère/bloc couleur jaune en gras
	else if(p_hdr -> typeflag == '3' || p_hdr -> typeflag == '4')
		strcat(a -> affichage, "\033[1;33m\033[48;5;236m");
	// Si c'est un executable couleur verte
	else if(strchr(p_hdr -> mode, '1') != NULL || strchr(p_hdr -> mode, '3') != NULL || 
			strchr(p_hdr -> mode, '5') != NULL || strchr(p_hdr -> mode, '7') != NULL)
		strcat(a -> affichage, "\033[1;32m");

	// Si c'est un fichier du repertoire choisi on affiche que le nom apres rep/...
	if(rep != NULL) {
		char * nom = malloc(strlen(name) - strlen(rep) + 1);
		strcpy(nom, &name[strlen(rep)]);
		strcat(a -> affichage, nom);
	}

	// Sinon on affiche son nom complet
	else
		strcat(a -> affichage, name);

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
