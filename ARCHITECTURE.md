# PROJET DE SYSTEME D'EXPLOITATION

## Stratégie adoptée

Pour ce projet nous avons décidé de commencer par l'implémentation des commandes demandées dans les fonctionnalités du shell. Chaque commande est représenté par un fichier `c`. Lors de l'avancement du projet, nous avons aussi commencé à réflechir sur l'implémentation du Makefile afin compiler et exécuter ces fichiers. Certains fichiers headers ont été ajoutés pour faciliter le codage des fichiers `.c`.

### Architecture logicielle
	
Le dépôt est constitué d'un fichier AUTHORS.md qui contient la liste des membres de l'équipe, un fichier .gitignore **(A REMPLIR)** et un répertoire contenant un Makefile, les fichiers C des commandes du shell et des fichiers headers.

### Structures de données et les algorithmes

Dans le répertoire "commandes":

* Le `Makefile` créé des éxécutables pour chaque commande possédant le nom du fichier C.

* Les fichiers C représente les commandes du shell pour les tarballs. Le nom de celles-ci est le nom de la commande avec `.c` à l'exception de tsh.c qui gère les commandes `cd` et `exit`

* Les fichiers headers facilitent l'implémentation des fichiers `.c`

 * `tar.h`: représente la structure d'un fichier tarball grace à une structure posix_header
 * `print.h`: implémente des fonctions qui facilite l'affichage dans la sortie standard
 * `cd.h`: **(A REMPLIR)**
 * `tar_nav.h` : **(A REMPLIR)**

### Algorithmes implémentés

Les commandes qui ont été implémentés

* `cd`:
	**(A REMPLIR)**

* `exit`:
	**(A REMPLIR)**

* `pwd`: Le fichier pwd.c affiche dans la sortie standard la concaténation du chemin du fichier exécuté et du tarball ou le repertoire du tarball si celui-ci existe. 

	**Entrée:** 
	```./pwd test.tar/nomfichier```

* `mkdir`: Le fichier mkdir.c permet de créer un ou plusieurs répertoires dans le tarball si celui ci n'existe pas déjà. Le nom entré ne doit pas dépasser 100 lettres. Le répertoire est ajouté à la fin du tarball. Il contient uniquement un bloc d'en-tête remplit des informations nécéssaires.
	
	**Entrée:** 
	```./mkdir rep1 rep_existant/rep2 rep3 ...```

* `rmdir`: Le fichier rmdir.c permet de supprimer un répertoire seulement s'il est vide. Pour le supprimer, le programme écrit par dessus le bloc d'en-tête du répertoire à supprimer. On déplace donc toutes les données se situant en dessous de celui-ci. Si le fichier est à la fin, alors il n'y a aucune données à déplacer et donc aucune données à écrire par dessus. Donc on diminue la taille du fichier pour suprrimer le répertoire.

	**Entrée:** 
	```./rmdir rep1/... rep2/...   ...```

* `cp`:
	**(A REMPLIR)**

* `ls`: Le fichier ls.c permet d'afficher le nom des fichiers dans le tarball :
	- sans afficher les fichiers appartenant à un répertoire si on souhaite afficher le tarball lui-même
	- en affichant uniquement le nom des fichiers appartenant au repertoire que l'on souhaite afficher sinon
	Le fichier conitent l'option `-l` pour afficher des informations supplémentaires qui à le même rôle que `ls -l` dans un répertoire.

	**Entrée:**
	```
	./ls test.tar
	``` 
	ou
	```
	./ls test.tar/rep/...
	``` 
	ou
	```
	./ls -l test.tar
	``` 
	ou
	```
	./ls -l test.tar/rep/...
	```
	
* `cat`:
	**(A REMPLIR)**

