#include <stdio.h>     // sscanf sprintf perror
#include <fcntl.h>     // open
#include <stdlib.h>    // exit getenv
#include <unistd.h>    // read close lseek write
#include <sys/types.h> // lseek
#include <string.h>    // strcmp strtok strchr
#include <time.h>      // localtime time
#include "tar.h"
#include "print.h"
#include "lib.h"

int main (int argc, char *argv[]) {
	// On suppose que la lecture se fait uniquement depuis le tar
	// Quatre formats possibles:
	// (1) ls
	// (2) ls rep1/ rep2/blablah/ rep3/ ...
	// (3) ls -l
	// (4) ls -l rep1/ rep2/blablah/ rep3/ ...

	// ======================================================================
	// 			      INITIALISATION
	// ======================================================================

	// Conditions des valeurs d'entrée
	if(argc < 1) {
		prints("\033[1;31mEntrée invalide\n\033[0m");
		exit(-1);
	}

	// S'il y a l'option "-l"
	int l = 0;
	int nbrep = 0;
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-l") == 0)
			l = 1;
		else
			nbrep++;
	}
	
	// ======================================================================
	// 			      OUVERTURE DU TAR
	// ======================================================================

	int fd;
	char * tar = getenv("tar");

	fd = open(tar, O_RDONLY);
	if (fd<0) {
		perror("\033[1;31mErreur lors de l'ouverture du tar\033[0m");
		exit(-1);
	}

	// ======================================================================
	// 	 			LECTURE DU TAR
	// ======================================================================

	char tampon[512];				// lecture du bloc stocke ici
	int n;							// nombre de caractres lus dans un tampon
	unsigned int size;				// taille du fichier lu
	struct posix_header * p_hdr;	// header du fichier
	int p;							// profondeur
	int start;

	affichage format = malloc(sizeof(affichage));	// affichage final
	init(format, 1);

	// S'il n'y a pas de repertoire en argument
	if(nbrep == 0)
		start = 0;

	// S'il y a au moins un repertoire en argument
	else
		start = 1;

	// ----------------------------------------------------------------------
	// 	 	     	PARCOURS DU TAR POUR CHAQUE ARGUMENT
	// ----------------------------------------------------------------------

	for (int i = start; i < argc; i++) {

		lseek(fd,0 ,SEEK_SET);

		char* rep = NULL;			// nom du repertoire
		int repexiste = 0;			// 1: Le repertoire a ete trouve

		affichage a = malloc(sizeof(affichage));	// affichage a la fin du traitement
		init(a, 100);

		// On evite "-l"
		if(strcmp(argv[i], "-l") != 0) {

			// Si on veut afficher un repertoire du tar
			if(i != 0)
				rep = argv[i];

			// ----------------------------------------------------------------------
			// 	 		       PARCOURS DU TAR
			// ----------------------------------------------------------------------

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
					// 	 	     REPERTOIRE A AFFICHER
					// ----------------------------------------------------------------------

					// Si le repertoire correspond au repertoire a afficher
					if(p_hdr->typeflag == '5' && rep != NULL) {
						if (strcmp(p_hdr -> name, rep) == 0) {
							repexiste = 1;
							p = profondeur(p_hdr) + 1;
						}
					}

					// ----------------------------------------------------------------------
					// 	 		STOCKAGE DES INFORMATIONS
					// ----------------------------------------------------------------------

					// Format ls / ls -l: Si le fichier est dans un repertoire on ne l'affiche pas
					// Format ls / ls -l rep1/ rep2/ ...: Sinon on affiche uniquement ses fichiers
					if((rep == NULL && profondeur(p_hdr) == 0) || (rep != NULL && repexiste == 1 && profondeur(p_hdr) == p && estDansRep(p_hdr -> name, rep) == 1)) {

						// On realloue de la memoire afin que l'affichage puisse tout contenir
						etendre (a, 100);

						// S'il y a l'option "-l"
						if(l == 1) { 

							// Type de fichier
							ptype(a, p_hdr-> typeflag);

							// Droits d'accès
							pdroit(a, p_hdr-> mode);

							// Liens physiques
							plink(a, tar, p_hdr);

							// Nom propriétaire
							char proprietaire[strlen(p_hdr-> uname)+2];
							sprintf(proprietaire, "%s ", p_hdr-> uname);
							ajout(a, proprietaire);

							// Nom du groupe
							char groupe[strlen(p_hdr-> gname)+1];
							sprintf(groupe, "%s ", p_hdr-> gname);
							ajout(a, groupe);

							// Taille en octets
							sscanf(p_hdr->size,"%o", &size);
							char taille[40];
							sprintf(taille, "%d  ", size);
							ajout(a, taille);

							// Date de dernière modification 
							long temps;
							sscanf(p_hdr-> mtime,"%lo", &temps);
							ptemps(a, temps);
						}

						// Nom du fichier
						afficheNom(a, p_hdr, rep, repexiste, l, tar);

					}
				}

				// On passe a l'entete suivante
				sscanf(p_hdr->size,"%o", &size);
				lseek(fd,((size + BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);
			}

			// ----------------------------------------------------------------------
			// 	 		FINALISATION DES INFORMATIONS A STOCKER
			// ----------------------------------------------------------------------

			// Si on ne rencontre pas le repertoire recherche
			if(rep!= NULL && repexiste == 0) 
				printsss("ls: impossible d'accéder à '", rep ,"': Aucun fichier ou dossier de ce type\n");

			// Sinon on affiche ce repertoire
			else {

				etendre(format, taille(a) + 100);

				if(nbrep > 1) {
					char repertoire[strlen(rep) + 2];
					sprintf(repertoire, "%s:\n", rep);
					ajout(format, repertoire);
				}				

				if(l == 1) {
					char totale[30];
					sprintf(totale, "total %d\n", total(tar, rep));
					ajout(format, totale);
				}

				ajout(format, afficher(a));

				if(i < argc - 1 && nbrep  > 0)
					ajout(format, "\n");
			}
		}
	
	}

	// ----------------------------------------------------------------------
	// 	 		AFFICHAGE DES INFORMATIONS
	// ----------------------------------------------------------------------

	prints(afficher(format));

	close(fd);
	exit(0);
}
