#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <limits.h>
#include "tar.h"

int main (int argc, char *argv[]){

	char bufdir [PATH_MAX + 1];

	//while(1){

		getcwd(bufdir,sizeof(bufdir));

		printf("\033[1;32m\n> %s\033[0m\n",bufdir);
	//}
	return 0;
}