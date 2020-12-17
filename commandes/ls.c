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
	int nbfich = 0;
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-l") == 0)
			l = 1;
		else
			nbfich++;
	}
	
	// ======================================================================
	// 			      OUVERTURE DU TAR
	// ======================================================================

	char * tar = getenv("tar");
	char * var_rep = NULL;

	// Si la variable d'environnement est dans un repertoire du tar
	if(strchr(tar, '/') != NULL) {
		strtok(tar, "/");
		char * var_r = strtok(NULL, "");
		var_rep = malloc(strlen(var_r) + 2);
		strcpy(var_rep, var_r);
		strcat(var_rep, "/");
	}

	int fd = open(tar, O_RDONLY);
	if (fd<0) {
		perror("ls: erreur lors de l'ouverture du tar");
		exit(-1);
	}

	// ======================================================================
	// 	 			LECTURE DU TAR
	// ======================================================================

	char tampon[512];					// lecture du bloc stocke ici
	int n;								// nombre de caractres lus dans un tampon
	unsigned int size;					// taille du fichier lu
	struct posix_header * p_hdr;		// header du fichier
	int start;							// debut de la boucle
	int end;							// fin de la boucle
	int a_length;				 		// Longueur du tableau a

	// S'il n'y a pas de fichier en argument
	if(nbfich == 0){
		start = 0; 
		end = 1;
		a_length = 1;
	}

	// S'il y a au moins un fichier en argument
	else{
		start = 1;
		end = argc;
		a_length = argc - start;
	}

	node_t ** a = init_array(a_length);			// Tableau de listes de headers pour l'affichage
	node_t ** list_tar = init_array(a_length);	// Stockage des headers du tar pour d'autres recherches auxilaires

	// ----------------------------------------------------------------------
	// 	 	     	PARCOURS POUR CHAQUE ARGUMENT
	// ----------------------------------------------------------------------

	for (int i = start; i < end; i++) {

		// On evite "-l"
		if(strcmp(argv[i], "-l") != 0) {

			char * arg = NULL;								// nom de l'argument adapter a la variable de l'environnement

			// ----------------------------------------------------------------------
			// 	 	 ADAPTATION DE L'ARGUMENT A LA VARIABLE D'ENVIONNEMENT
			// ----------------------------------------------------------------------

			// Si il n'y a pas de repertoire/fichier en argument et que
			// la variable d'environnement se trouve dans un repertoire du tar
			if(i == 0 && var_rep != NULL)
				arg = var_rep;

			// Si on veut afficher un repertoire/fichier en argument
			if(i != 0) {
								
				arg = malloc(strlen(argv[i]) + 1);
				strcpy(arg, argv[i]);

				// Si la variable d'environnement se trouve dans un repertoire du tar
				if (var_rep != NULL) {
					char * nouv_arg = malloc(strlen(var_rep) + strlen(arg) + 1);
					strcpy(nouv_arg, var_rep);
					strcat(nouv_arg, arg);
					arg = realloc(arg, strlen(nouv_arg) + 1);
					strcpy(arg, nouv_arg);
				}
			}

			// ----------------------------------------------------------------------
			//			       PARCOURS DU TAR
			// ----------------------------------------------------------------------

			while(1) {

				// Lecture du bloc
				n = read(fd, &tampon, BLOCKSIZE);
				if(n<0) {
					perror("ls: Erreur lors de la lecture du tar");
					exit(-1);
				}
				if(n == 0) break;

				// Extraction des informations
				p_hdr = (struct posix_header*)tampon;

				// On evite les deux blocs finaux formés uniquement de zéros
				if(strlen(p_hdr-> name) != 0) {

					// On ajoute à la liste du tar le header
					list_tar[i-start] = add(list_tar[i-start], p_hdr, NULL);

					// ----------------------------------------------------------------------
					// 	 	     REPERTOIRE ENTREE SANS '/' TROUVE
					// ----------------------------------------------------------------------

					// Variables temporaires

					if(arg != NULL) {

						// Si l'argument entree est un repertoire sans '/' a la fin
						if(p_hdr-> typeflag == '5' && arg[strlen(arg) - 1] != '/') {

							char * name = malloc(strlen(arg) + 2);
							strcpy(name, arg);
							strcat(name, "/");
						
							// Si le repertoire correspond a l'argument
							// On redefinie de façon permanente arg comme un repertoire
							if (strcmp(p_hdr -> name, name) == 0) {
								arg = malloc(strlen(name) + 1);
								strcpy(arg, name);
							}
						}
					}

					// ----------------------------------------------------------------------
					// 	 		STOCKAGE DES INFORMATIONS
					// ----------------------------------------------------------------------

					// On affiche uniquement les fichiers dans le tar/repertoire de même profondeur
					if((arg == NULL && profondeur(p_hdr) == 0) || 
					   (arg != NULL && a[i-start] != NULL && profondeur(p_hdr) == get_profondeur(get_head(a[i-start])) + 1 && estDansRep(p_hdr -> name, arg) == 1) || 
					   (arg != NULL && a[i-start] == NULL && egaux(p_hdr -> name, arg) == 1)) {

						// On ajoute à la liste de arg le header
						a[i-start] = add(a[i-start], p_hdr, argv[i]);
					}
				}
			
				// On passe a l'entete suivante
				sscanf(p_hdr->size,"%o", &size);
				lseek(fd,((size + BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);
			}
		}

		lseek(fd,0 ,SEEK_SET);
	}

	// ======================================================================
	// 	 		FINALISATION DES INFORMATIONS A STOCKER
	// ======================================================================

	affichage format = malloc(sizeof(affichage));	// affichage final
	init(format);								// initialisation de l'affichage

	// AFFICHAGE SANS ARGUMENT
	if(nbfich == 0) {

		// Conversion des informations				
		node_t * node = get_head(a[0]);					// liste de fichiers à afficher
		int total = 0;		 	 	 			 		// total de l'epsace occupé

		// Variable d'environnement stocké en 1ere position
		if(var_rep != NULL)
			node = get_next(node);

		while(node != NULL) {
			afficher(format, node, list_tar[0], l, &size, var_rep);
			total += 1 + ((size + BLOCKSIZE - 1) >> BLOCKBITS);
			node = get_next(node);
		}

		// Ajout dans l'affichage
		if(l == 1) {
			prints("total ");
			printd(total);
			prints("\n");
		}
	}

	// AFFICHAGE DE CHAQUE ARGUMENT
	else {
	
		// ----------------------------------------------------------------------
		//	 	 	     		 TRI DES LISTES
		// ----------------------------------------------------------------------

		// 1er tableau: fichiers/repertoires non existants
		// 2e  tableau: fichiers existants
		// 3e  tableau: repertoires existants

		int length1 = 0; 
		int length2 = 0;
		int length3 = 0;

		// Calcul de la taille des tableaux
		for(int i = 0; i < a_length ; i++) {
			if(a[i] != NULL) {
				node_t * head = get_head(a[i]);
				if(get_header(head).typeflag == '5')
					length3++;
				else
					length2++;
			}
			else
				length1++;
		}

		// Initialisation du tableau a 3 dimension
		node_t ** array[3];
		array[0] = init_array (length1);	// fichiers/repertoires non existants
		array[1] = init_array (length2);	// fichiers existants
		array[2] = init_array (length3);	// repertoires existants
		node_t ** l_tar [2];
		l_tar[0] = init_array(length2);		// recherches auxiliaires pour fichiers
		l_tar[1] = init_array(length3);		// recherches auxiliaires pour repertoires

		// Remplissage du tableau
		int ind1 = 0;
		int ind2 = 0;
		int ind3 = 0;
		memset(tampon, '\0', 512);
		struct posix_header * p = (struct posix_header*)tampon; // ne sera jamais utilisé

		for(int i = 0; i < a_length ; i++) {
			if(a[i] != NULL) {
				node_t * head = get_head(a[i]);
				node_t * head_tar = get_head(list_tar[i]);

				// Si c'est un repertoire
				if(get_header(head).typeflag == '5') {
					array[2][ind3] = head;
					l_tar[1][ind3] = head_tar;
					ind3++;
				}
				// Sinon c'est un fichier
				else {
					array[1][ind2] = head;
					l_tar[0][ind2] = head_tar;
					ind2++;
				}
			}
			// Si le fichier/repertoire n'existe pas
			else{
				array[0][ind1] = add(array[0][ind1], p, argv[i+start]);
				ind1++;
			}
		}

		// ----------------------------------------------------------------------
		//	 	 	     	CONVERSION DES INFORMATIONS
		// ----------------------------------------------------------------------

		// FICHIER/REPERTOIRES NON TROUVES
		for(int i = 0; i < length1; i++) {
			node_t *  node = get_head(array[0][i]);
			if(strcmp(get_argv(node), "-l") != 0)
				printsss("ls: impossible d'accéder à '", get_argv(node) ,"': Aucun fichier ou dossier de ce type\n");
		}
		
		// FICHIERS
		for (int i = 0; i < length2 ; i++) {
			node_t * node = get_head(array[1][i]);
			afficher(format, node, l_tar[0][i], l, &size, var_rep);
		}
		if(length3 > 0 && length2 > 0)
			ajout(format, "\n");

		// REPERTOIRES
		for (int i = 0; i < length3 ; i++) {

			affichage aff = malloc(sizeof(affichage));
			init(aff);
			node_t * node = get_head(array[2][i]);
			int total = 0;

			// Nom du repertoire
			if(length3 > 1 || length2 > 0){
				char repertoire[strlen(get_argv(node)) + 2];
				sprintf(repertoire, "%s:\n", get_argv(node));
				ajout(format, repertoire);
			}

			// Repertoire stocké en 1ere position
			node = get_next(node);

			// Ajout dans l'affichage chaque fichiers du repertoire
			while(node != NULL) {
				afficher(aff, node, l_tar[1][i], l, &size, get_header(get_head(node)).name);
				total += 1 + ((size + BLOCKSIZE - 1) >> BLOCKBITS);
				node = get_next(node);
			}

			// Total
			if(l == 1) {
				int n = strlen_int(total);
				char totale[n + strlen("total \n")];
				sprintf(totale, "total %d\n", total);
				ajout(format, totale);
			}

			// Ajout dans l'affichage final
			ajout(format, to_string(aff));

			if(i < length3 - 1)
				ajout(format, "\n");
		}		
	}

	// ----------------------------------------------------------------------
	// 	 		AFFICHAGE DES INFORMATIONS
	// ----------------------------------------------------------------------

	prints(to_string(format));
	close(fd);
	exit(0);
}
