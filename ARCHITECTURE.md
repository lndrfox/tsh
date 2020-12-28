# PROJET DE SYSTEME D'EXPLOITATION

## Fonctionnalités disponibles

Voici les fonctionnalités disponibles du tsh: 

* Affichage du nom de l'utilisateur, nom de la machine et du répertoire courant au début du prompt.

* Fonctionnement normal des commandes lorsqu’un tarball n'est pas impliqué.

* Fonctionnement des commandes `ls` (nottament avec l'option -l), `cp` et `rm` (nottament avec l'option -r) , `mv` , `rmdir`, `cat` , `pwd`, `cd` et `exit` lorsque des tarballs sont impliqués. Les options sont uniquement prises en compte lorsqu'elles sont entrées avant n'importe quel argument. La commande `cd` sans argument renvoit dans le répertoire `home/`.

* Fonctionnement normal des commandes lorsqu'on leur donne plusieurs arguments et que des tarballs sont impliqués ainsi que lorsqu'on on leur donne plusieurs arguments, certains impliquants des tarballs, d'autres non.

* Fonctionnement des redirections `>` , `>>` , `2>` , `2>>` , `<` que des tarballs soient impliqués ou non. Fonctionnement des redirections multiples que des tarballs soient impliqués ou non.

* Fonctionnement normal des combinaisons de commande que des tarballs soient impliqué ou non, y compris plusieurs combinaisons de commandes à la suite.

## Architecture

tsh.c est le point central de notre architecture. Le tsh va s'occuper de récupérer ce qui est entré par l'utilisateur grâce a la librairie readline. A l'aide de fonctions disponibles dans tar_nav.c , il va par la suite (expliqué plus en détail dans l'explication de tsh) s'occuper de parser le prompt obtenu permettant ainsi de traiter si nécessaire la présence de combinaisons de commandes et de redirection. Dernièrement il va appeler la ou les commandes nécéssaires avec leurs arguments sur un ou des processus fils.

Chaque commande devant fonctionner sur les tarballs est gérer dans un fichier à son nom possédant un main afin de pouvoir être appelée avec exec depuis le tsh à l'exception de `cd` et `exit` qui sont inhérent au shell. `exit` se contente de mettre à faux la condition qui est vérifiée à chaque tour de boucle du shell. 

Afin de gèrer `cd` et les tarballs nous avons mis au point un système de surcouche. Une variable d'environnent nommée "tar" contient le répertoire courant dans les tarballs. Si nous ne sommes actuellement pas dans un tarball alors elle contient uniquement "". `cd`, qui est une fonction codée dans cd.c (afin de mieux séparer le code, mais ce fichier ne contient pas de main) à le même comportement que le `cd` du bash quand les tarballs ne sont pas impliqués. Quand les tarballs sont impliqués, il s'assure de modifier la variable d'environnement "tar" en conséquence.

##  Explication de tsh.c

![](tshDiagramme.png)

Le diagramme ci-dessus illustre le fonctionnement de tsh.c sans entrer dans les détails. La boucle tsh continue de tourner dans que l'entier run n'a pas été mit à 0,ce qui n'a lieu que lorsque la commande `exit` est appelée. Grâce a la fonction `get_line()` qui fais appel à la librairie `readline` on récupère le texte entré par l'utilisateur.Après cela, on passe l'entrée récupèrée à la fonction `redir()` qui se charge de traiter les potentielles redirections et d'enlever le texte relatif aux redirections du prompt.

On passe ensuite le prompt à la fonction `exec_pipe()` qui va se charger de crèer le nombre de processus nécéssaire pour traiter les combinaisons de commandes correctement ( si elles sont présente ) et de les relier par des tubes anonymes. Une fois ceci fait, ou de manière immédiate si il n'y a aucune combinaison de commnades à traiter, la fonction `exec_custom()` est appelée, elle se charge de crèer un processus fils puis appelle `exec_tar_or_bin()`. Une fonction qui prend en argument un ` char ** args` contenant le nom de la fonction et ses arguments/options et terminan par NULL ainsi qu'un boolean indiquant si l'ont peut exécuter directement la commande "normale" avec `exec` ou si l'on doit procèder a quelque vérification supplémentaire pour savoir si
nous devons à la place exécuter sa version spécifique aux tarballs.

### Les redirections 

Attardons nous sur le fonctionnement de la fonction `redir_out()`, la fonction `redir_in()` fonctionne de manière très similaire.

Cette fonction va parcourir le `char * prompt` fourni en argument et chercher la présence d'un symbole de redirection `>` . Elle va par la suite en analysant les caractères adjacents, déterminer si il s'agot d'une redirection `>` , `>>` , `2>` ou `2>>` et va crèer une chaine de caractère path contenant le texte 
présent entre le symbole de redirection trouvé et le prochain symbol de redirection dans `prompt` ou la fin de `prompt`. Par exemple, "commande > blabla 2> tata" nous fournira commande chaîne "blabla". La fonction prend bien soin de supprimer les espaces superflu. Une fois ceci fait, on va ourvir un nouveau descripteur avec `open`, les paramètres du descripteur dépendant de quel symbole de redirection nous traitons  ( `>` ou `>>`) puis nous
redirigeons la sortie qui nous intéresse, a savoir `STDOUT_FILENO` ou `STDERR_FILENO` selon les circonstances, sur le descripteur que nous venons d'ouvrir, à l'aide de `dup2`. Avant d'appeller `dup2` nous sauvegardons le descripteur remplacé dans une variable global afin qu'il puisse être restauré plus tard. Une fois ceci fait nous enlevons du prompt le symbole de redirection aisni que le chemin de redirection a l'aide d'un `memmove` ( fonction auxiliaire `str_cut`).

Cette boucle va se répèter tant que nous trouvons un symbole `>` ou que nous n'avons pas atteint la fin de prompt. Le processus est très similaire pour `redir_in()` mis à part que le symbole que nous recherchons est `<` et qu'il y a moins de paramètres à prendre en compte.

Le cas ou le fichier vers lequel la redirection doit avoir lieu se situe dans un tarball est un peu plus complexe. En effet, il est impossible de procèder à un simple open du fichier. Voici la stratégie que nous avons décidée d'adopter. Encore une fois nous nous attarderons plus longuement sur l'implémentation dans `redir_out()` car elle est très similaire à celle de `redir_in`. Lorsque la chaîne de caractère représentant le chemin vers lequel la redirecion doit avoir lieu se situe dans un tarball ( ce que nous pouvons déterminer grâce à la fonction auxiliaire `goes_back_in_tar()` ) nous copions le contenu du fichier hors du tar, dans le premier répertoire courant qui n'est pas un tarball, et avec un nom fixe ( `redir_out` ou `redir_err`). Nous utilisons un nom fixe afin d'éviter les conflits entre deux fichiers différents entre le tarball et l'exterieur mais portant le même nom.
c'est ensuite ce fichier `redir_out/redir_err` qui sera ouvert lors du open, et le chemin du fichier dans le tarballs sera mémorisé dans une variable globale. Par la suite, lors de la réinitialisation des descripteurs, on copie de nouveau le contenu de `redir_out/redir_err` dans le tar mais sous son vrai nom. Puis on supprime le fichier devenu inutile. 

## Algorithmes implémentés

Les commandes qui ont été implémentés

* `mkdir`: 

* `rmdir`: 

* `cp`: 

* `mv`:

* `rm`:
	
* `ls`: 
	
* `cat`: 

* `pwd`:
