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

	if(name != NULL && rep != NULL) {

		char * repertoire = malloc(strlen(rep) +1);
		char * isrep = malloc(strlen(rep) +1);
		strncpy(repertoire, rep, strlen(rep));
		strncpy(isrep, name, strlen(rep));

		if(strcmp(isrep, repertoire) == 0)
			return 1;
	}
 
	return 0;
}

// Retourne si deux string sont egaux meme si NULL est en argument
int egaux(char * s1, char * s2) {

	if(s1 != NULL && s2 != NULL) {
		if(strcmp(s1, s2) == 0)
			return 1;
	}

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
// 	 				LISTE DE HEADERS
// ----------------------------------------------------------------------

typedef struct node {
	char * argv;
    struct posix_header p;
    int profondeur;
    struct node * next;
    struct node * head;
} node_t;

// Ajout à la fin de la liste
node_t * add(node_t * prev, struct posix_header* p, char * argv) {
	node_t * node = (node_t *) malloc(sizeof(node_t));
	node -> argv = argv;
	node -> p = *p;
	node -> profondeur = profondeur(p);
	node -> next = NULL;	

	if(prev == NULL)
		node -> head = node;

	else {	
		prev -> next = node;
		node -> head = prev -> head;
	}

	return node;
}

// Retourne l'argument associé a la liste
char * get_argv(node_t * node) {
	if(node != NULL)
		return node -> argv;
	return "";
}

// Retourne la header du noeud
struct posix_header get_header(node_t * node) {
	if(node != NULL)
		return node -> p;
	char tampon[512];
	memset(tampon, '\0', 512);
	struct posix_header * p = (struct posix_header*)tampon;
	return *p;
}

int get_profondeur(node_t * node) {
	if(node != NULL)
		return node -> profondeur;
	return -2;
}

// Retourne le noeud suivant
node_t * get_next(node_t * node) {
	if(node != NULL)
		return node -> next;
	return NULL;
}

// Retourne la tête de la liste
node_t * get_head(node_t * node) {
	if(node != NULL)
		return node -> head;
	return NULL;
}

// Tableau de listes de headers

node_t ** init_array (int size) {
	node_t ** a = malloc(sizeof(node_t) * size);
	for (int i = 0; i < size; i++) 
		a[i] = NULL;
	return a;
}

// Retourne si le fichier existe bien dans le tar
int existant (node_t * n, char * file, char * rep) {

	char * f;						// Format du nom
	node_t * node = get_head(n);	// Liste des fichiers du tar

	// Si le fichier se trouve dans un repertoire du tar
	if(rep != NULL) {
		f = malloc(strlen(file) + strlen(rep) + 1);
		strcpy(f, rep);
		strcat(f, file);
	}

	// Sinon on ne change pas le nom
	else {
		f = malloc(strlen(file) + 1);
		strcpy(f, file);
	}

	// On parcours la liste representant le tar
	while(node != NULL) {
		if(strcmp(node -> p.name, f) == 0) 
			return 1;
		node = get_next(node);
	}

	return 0;
}

// ----------------------------------------------------------------------
// 	 	     		AFFICHAGE TAR
// ----------------------------------------------------------------------

typedef struct affichage {
	char * affichage;
	int size;
} a;

char * to_string(a * a) {
	if (a -> affichage == NULL)
		return "";
	return a -> affichage;
}

void init(a * a) {
	a -> affichage = NULL;
	a -> size = 0;
}

void ajout(a * a, char * ajout) {
	if(a == NULL) {
		a -> affichage = malloc(strlen(ajout) + 1);
		a -> size = strlen(ajout) + 1;
		memset(a -> affichage , '\0', a -> size);
	}
	else{
		a -> affichage = realloc(a -> affichage, (a -> size) + strlen(ajout) + 1);
		memset(&(a -> affichage)[a -> size] , '\0', strlen(ajout) + 1);
		a -> size = (a -> size) + strlen(ajout) + 1;
	}
	strcat(a -> affichage, ajout);
}

// Affiche la date
void ptemps(a * a, long temps) {
	struct tm * tempsformat = localtime(&temps);
	long tactuel = time(NULL);

	// Mois
	switch (tempsformat->tm_mon) {
		case 0: ajout(a, "janv."); break;
		case 1: ajout(a, "fevr."); break;
		case 2: ajout(a, "mars "); break;
		case 3: ajout(a, "avril"); break;
		case 4: ajout(a, "mai  "); break;
		case 5: ajout(a, "juin "); break;
		case 6: ajout(a, "juil."); break;
		case 7: ajout(a, "août "); break;
		case 8: ajout(a, "sept."); break;
		case 9: ajout(a, "oct. "); break;
		case 10:ajout(a, "nov. "); break;
		case 11:ajout(a, "dec. "); break;

	}

	// Jour
	int n = strlen_int(tempsformat -> tm_mday);
	char jour[n + 2];
	sprintf(jour, " %d ", tempsformat -> tm_mday);
	ajout(a, jour);

	// Si le fichier a ete modifie il y a moins de 6 mois
	if(tactuel-temps < 60*60*24*183) {
		char hour[3];
		char min[3];
		char time[6];

		if(tempsformat -> tm_hour < 10)
			sprintf(hour, "0%d", tempsformat -> tm_hour);
		else 
			sprintf(hour, "%d", tempsformat -> tm_hour);
		if(tempsformat -> tm_min < 10) 
			sprintf(min, "0%d", tempsformat -> tm_min);
		else 
			sprintf(min, "%d", tempsformat -> tm_min);

		sprintf(time, "%s:%s", hour, min);
		ajout(a, time);
	}
	else{
		char annee[4];
		sprintf(annee, "%d", tempsformat -> tm_year + 1900);
		ajout(a, annee);
	}
	ajout(a, " ");
}

// Affiche le type du fichier
void ptype(a * a, char t) {
	switch(t) {
			case '2':  ajout(a, "l"); break;
			case '3':  ajout(a, "c"); break;
			case '4':  ajout(a, "b"); break;	
			case '5':  ajout(a, "d"); break;
			case '6':  ajout(a, "p"); break;
			default :  ajout(a, "-"); break;
		}
}

// Affiche les droits d'acces du fichier
void pdroit(a * a, char *d) {
	for(int i = 4; i < 7; i++) {
		switch(d[i]) {
			case '0': ajout(a, "---"); break;
			case '1': ajout(a, "--x"); break;
			case '2': ajout(a, "-w-"); break;
			case '3': ajout(a, "-wx"); break;
			case '4': ajout(a, "r--"); break;
			case '5': ajout(a, "r-x"); break;
			case '6': ajout(a, "rw-"); break;
			case '7': ajout(a, "rwx"); break;
		}
	}
}

// Affiche le nombre de liens physiques du fichier
void plink(a * a, node_t * tar, struct posix_header hdr) {

	node_t * node = get_head(tar);
	char *file =   hdr.name;
	char *file_l = hdr.linkname;;
	char *f =  "";
	char *fl = "";
	int compt = 1;

	// On parcours la liste representant le tar
	while(node != NULL) {
		f =  node -> p.name;
	    fl = node -> p.linkname;

		// Comparaison entre le fichier mis en parametre et tous les autres fichiers
	 	if(strlen(fl) > 0 && node -> p.typeflag != '2') {
	 		if((strcmp(file, f)==0) || (strcmp(file, fl)==0) || (strcmp(file_l, f)==0) || (strcmp(file_l, fl)==0))
				compt ++;
	 	}
		node = get_next(node);
	}

	int n = strlen_int(compt);
	char hardlink[n + 2];
	sprintf(hardlink, " %d ", compt);
	ajout(a, hardlink);
}

// Affiche le nom du fichier
void afficheNom(a * a, node_t * node, node_t * tar, int option, char * var_rep) {

	struct posix_header hdr = get_header(node); 
	char * name = hdr.name;
	char n[strlen(hdr.name)];
	int existe = existant(tar, hdr.linkname, var_rep);
	int couleur = 0;

	// Si c'est un repertoire: couleur bleu
	if(hdr.typeflag == '5'){
		ajout(a, "\033[1;34m");
		couleur = 1;
		// Supprimer '/' du nom
		strcpy(n, hdr.name);
		n[strlen(n) - 1] = '\0';
		name = n;
	}

	// Si c'est un lien symbolique couleur cyan/rouge
	else if(hdr.typeflag == '2') {
		if(existe == 1)
			ajout(a, "\033[1;36m");
		else
			ajout(a, "\033[1;31m\033[48;5;236m");
		couleur = 1;
	}

	// Si c'est un tube couleur jaune
	else if(hdr.typeflag == '6'){
		ajout(a, "\033[0;33m\033[48;5;236m");
		couleur = 1;
	}

	// Si c'est un fichier spécial caractère/bloc couleur jaune en gras
	else if(hdr.typeflag == '3' || hdr.typeflag == '4'){
		ajout(a, "\033[1;33m\033[48;5;236m");
		couleur = 1;
	}

	// Si c'est un executable couleur verte
	else if(strchr(hdr.mode, '1') != NULL || strchr(hdr.mode, '3') != NULL || 
			strchr(hdr.mode, '5') != NULL || strchr(hdr.mode, '7') != NULL){
		ajout(a, "\033[1;32m");
		couleur = 1;
	}

	// Si c'est un fichier du repertoire choisi on affiche que le nom apres rep/...
	if(var_rep != NULL) {
		char * nom = malloc(strlen(name) - strlen(var_rep) + 1);
		strcpy(nom, &name[strlen(var_rep)]);
		ajout(a, nom);
	}

	// Sinon on affiche son nom complet
	else{
		ajout(a, name);
	}

	// Renitialise la couleur
	if(couleur == 1)
		ajout(a, "\033[0m");

	// Si c'est un symbolique et qu'il y a l'option "-l" on affiche le fichier sur lequel il pointe 
	if(hdr.typeflag == '2' && option == 1) {
		ajout(a, " -> ");
		if(existe == 0) ajout(a, "\033[1;31m\033[48;5;236m");
		ajout(a, hdr.linkname);
		if(existe == 0) ajout(a, "\033[0m");
	}

	// On ajoute un retour a la ligne
	ajout(a, "\n");
}

// Affichage du fichier
void afficher (a * aff, node_t * node, node_t * tar, int l, unsigned int * size, char * var_rep) {

	// S'il y a l'option "-l"
	if(l == 1) { 

		// Type de fichier
		ptype(aff, node -> p.typeflag);

		// Droits d'accès
		pdroit(aff, node -> p.mode);

		// Liens physiques
		plink(aff, tar, node -> p);

		// Nom propriétaire
		char proprietaire[strlen(node -> p.uname)+2];
		sprintf(proprietaire, "%s ", node -> p.uname);
		ajout(aff, proprietaire);

		// Nom du groupe
		char groupe[strlen(node -> p.gname)+1];
		sprintf(groupe, "%s ", node -> p.gname);
		ajout(aff, groupe);

		// Taille en octets
		sscanf(node -> p.size, "%o", size);
		char taille[40];
		sprintf(taille, "%d  ", *size);
		ajout(aff, taille);

		// Date de dernière modification 
		long temps;
		sscanf(node -> p.mtime,"%lo", &temps);
		ptemps(aff, temps);
	}

	// Nom du fichier
	afficheNom(aff, node, tar, l, var_rep);
}