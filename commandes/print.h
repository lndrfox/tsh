#ifndef PRINT_H
#define PRINT_H

/* OBLIGATOIRE DANS LE FICHIER
#include <stdio.h>	   // sprintf
#include <stdlib.h>    // exit
#include <unistd.h>    // read close lseek write
#include <string.h>    // strlen
*/

// Impression d'un char*
void prints(char* s) {
	char buf[strlen(s)];
	strcpy(buf, s);
	int n;
	n = write(STDOUT_FILENO , &buf, strlen(s));
	if(n<0) {
		perror("\033[1;31mErreur d'impression");
		exit(-1);
	}
}

// Impression d'un int
void printd(int s) {
	int num = s;
	int n = 1;
	while(num > 10) {
		num = num/10;
		n++;
	}
	char str[n];
	if(sprintf(str, "%d", s) <0) {
		perror("\033[1;31mMauvais format ('d') de print ");
		exit(-1);
	}
	prints(str);
}

// Impression d'un char
void printc(char s) {
	char str[2];
		if(sprintf(str, "%c", s) < 0) {
		perror("\033[1;31mMauvais format ('c') de print");
		exit(-1);
	}
	prints(str);
}

// Impression d'un int en base octal
void printo(int s) {
	char str[12];
		if(sprintf(str, "%o", s) < 0) {
		perror("\033[1;31mMauvais format ('o') de print");
		exit(-1);
}
	prints(str);
}

#endif
