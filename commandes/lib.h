#ifndef LIB_H
#define LIB_H


// UTILITAIRE

extern int estDansRep(char * name, char * rep);
extern int egaux(char * s1, char * s2);
extern int profondeur (struct posix_header * p_hdr);

// LISTE DE HEADERS

typedef struct node node_t;
extern node_t * head(struct posix_header* p, char * argv);
extern node_t * add(node_t * prev, struct posix_header* p, char * argv, int todo);
extern int get_todo(node_t * node);
extern char * get_argv(node_t * node);
extern struct posix_header get_header(node_t * node);
extern int get_profondeur(node_t * node);
extern node_t * get_next(node_t * node);
extern node_t * get_head(node_t * node);
extern node_t ** init_array (int size);
extern int existant (node_t * n, char * file, char * rep);

// AFFICHAGE

typedef struct affichage * affichage;
extern char * to_string(affichage a);
extern void init(affichage a);
extern void ajout(affichage a, char * ajout);
extern void ptemps(affichage a, long temps);
extern void ptype(affichage a, char t);
extern void pdroit(affichage a, char * d);
extern void plink(affichage a, node_t * tar, struct posix_header hdr);
extern void afficheNom(affichage a, node_t * node, node_t * tar, int option, char * var_rep);
extern void afficher (affichage aff, node_t * node, node_t * tar, int l,  unsigned int * size, char * var_rep);

#endif