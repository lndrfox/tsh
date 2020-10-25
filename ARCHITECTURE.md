# PROJET DE SYSTEME D'EXPLOITATION

## Stratégie adoptée

Pour ce projet nous avons décidé de commencer par l'implémentation des commandes demandées dans les fonctionnalités du shell. Chaque commande est représenté par un fichier `.c`, à l'exception de cd puisqu'il s'agit d'une commande codée directement dans le Shell et qui ne peut être appelée avec exec pour fonctionner correctement . Lors de l'avancement du projet, nous avons aussi commencé à réflechir sur l'implémentation du `Makefile` afin compiler et exécuter ces fichiers. Certains fichiers headers ont été ajoutés pour faciliter le codage des fichiers `.c`.

### Architecture logicielle
	
Le dépôt est constitué d'un fichier AUTHORS.md qui contient la liste des membres de l'équipe, un fichier .gitignore et un répertoire contenant un Makefile, les fichiers `.c` des commandes du shell et des fichiers headers.

### Structures de données et les algorithmes

Dans le répertoire "commandes":

* Le `Makefile` créé des éxécutables pour chaque commande possédant le nom du fichier `.c`.

* La partie centrale du programme est dans le fichier `tsh.c` On y trouve une boucle qui lis les commandes entrée par l'utilisateur à chaque itération à l'aide de la librairie readline. La fontion parse decompose ensuite les arguments et a l'aide d'exec appelle la bonne commande selon que des tars soient impliqués ou non. 

* Les fichiers `.c` représente  chacun une commande du shell pour les tarballs à l'exception de `tsh.c` qui gère les commandes `cd` et `exit`.
 
* Afin de gèrer le déplacement dans les tar en plus de path "normal" accesible via getcwd, nous stockons le chemin dans les tar dans la variable d'environnement "tar" qui peut ainsi être accèdée par chaque processus fils.

* Les fichiers headers facilitent l'implémentation des fichiers `.c`.

  * `tar.h`: représente la structure de l'entête d'un fichier dans un tarball grâce à une structure posix_header.
  * `print.h`: implémente des fonctions qui facilitent l'affichage dans la sortie standard.
  * `cd.h`: contient la fonction cd en dehors des tar et un début de fonctionnement de cd dans les tar (pour l'instant il permet uniquement d'accèder a un tar avec cd fichier.tar ou d'en sortir avec cd ..).
  * `tar_nav.h` : contient des fonctions aidant à la navigation dans les tar et la gestion des path dans les tar.

### Algorithmes implémentés

Les commandes qui ont été implémentés

* `cd`: C'est une commande codée directement dans le Shell. Nous stockons le chemin dans les tar dans la variable d'environnement "tar" qui peut être accèdée par chaque processus fils.

* `exit`
	Dans le main, la boucle du shell tourne tant qu'un entier run est a 1. La fonction exit qui est implémentée directement dans le shell tsh met run a 0 et interrompt
ainsi tsh.

* `pwd`: Le fichier pwd.c affiche sur la sortie standard la concaténation du chemin du fichier exécuté et du tarball ou le repertoire du tarball si celui-ci existe. 

	**Entrée:** 
	`./pwd test.tar/nomfichier`

* `mkdir`: Le fichier mkdir.c permet de créer un ou plusieurs répertoires dans le tarball si celui-ci n'existe pas déjà. Le nom entré ne doit pas dépasser 100 lettres. Le répertoire est ajouté à la fin du tarball. Il contient uniquement un bloc d'en-tête remplit des informations nécéssaires.
	
	**Entrée:** 
	`./mkdir rep1 rep_existant/rep2 rep3 ...`

* `rmdir`: Le fichier rmdir.c permet de supprimer un répertoire si et seulement s'il est vide. Pour le supprimer, on déplace toutes les données se situant en dessous de celui-ci à la suite des données se situant avant. Si le répertoire est à la fin, alors il n'y a aucune donnée à déplacer. Donc on diminue la taille du fichier pour suprrimer le répertoire. Pour l'instant rmdir.c n'est pas fonctionnelle sur des entrées `rep1/rep2/...`.

	**Entrée:** 
	`./rmdir rep1/... rep2/...   ...`

* `cp`: Le fichier cp.c permet de copier un fichier d'un tarball vers un autre ou d'un fichier en dehors du tarball vers un tarball. La commande est utilisé selon la manière dont elle est appelé.

  	**Entrée:**


	Si l'on veut copier un fichier du tarball vers un autre tarball: 
	`cp fichier_à_copier test.tar/nomfichier`

	Si l'on veut copier un fichier du tarball en dehors du tarball: 
	`cp fichier_à_copier path/nom_de_la_copie`

	Si l'on veut copier un fichier en dehors du tarball vers un tarball: 
	`cp path/fichier_à_copier nom_de_la_copie`
  	
	

* `ls`: Le fichier ls.c permet d'afficher le nom des fichiers dans le tarball :
  * sans afficher les fichiers appartenant à un répertoire si on souhaite afficher le tarball lui-même
  * en affichant uniquement le nom des fichiers appartenant au repertoire que l'on souhaite afficher sinon
Le fichier conitent l'option `-l` pour afficher des informations supplémentaires qui à le même rôle que `ls -l` dans un répertoire.
Pour l'instant ls.c n'est pas fonctionnelle sur des entrées `test.tar/rep1/rep2/...`.

	**Entrée:**


	`ls test.tar` ou

	`ls test.tar/rep/` ou

	`ls -l rep` ou

	`ls -l test.tar/rep/`
	
* `cat`: Quand appelée avec des arguments ou dans une arborescence contentant un `.tar`, cat affiche le contenu des fichiers passé en argument.

	**Entrée:**
	`./cat fichiertar.tar fichier autrefichier ...`

