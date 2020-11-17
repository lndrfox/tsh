#ifndef LIB_H
#define LIB_H


// UTILITAIRE

extern int estDansRep(char * name, char * rep);
extern int profondeur (struct posix_header * p_hdr);

// AFFICHAGE

typedef struct affichage * affichage;
extern void init(affichage a, int size);
extern void etendre(affichage a, int size);
extern char * afficher(affichage a);
extern int taille(affichage a);
extern void ajout(affichage a, char * ajout);
extern void ptemps(affichage a, long temps);
extern void ptype(affichage a, char t);
extern void pdroit(affichage a, char * d);
extern void plink(affichage a, char * tar, struct posix_header * p_hdr1);
extern void afficheNom(affichage a,struct posix_header * p_hdr, char* rep, int repexiste, int option, char * tar);
extern int total(char * tar, char * rep);

#endif