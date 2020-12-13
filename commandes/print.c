#include <stdio.h>	   // sprintf
#include <stdlib.h>    // exit
#include <unistd.h>    // read close lseek write
#include <string.h>    // strlen
#include "print.h"

// Calcul la longueur d'un int pour le transofmer en string
int strlen_int(int num) {
	int n = 1;
	while(num > 10) {
		num = num/10;
		n++;
	}
	return n;
}

void print_stderr(char * buf){

	int n=write(STDERR_FILENO,buf,strlen(buf));
	if(n<0){

		perror("\033[1;31mErreur d'impression");
		exit(-1);
	}
}

/*Affiche l'erreur tsh: where: who: what*/
void print_error(char * where, char * who, char * what){

	print_stderr("tsh: ");
	if(where!=NULL)
	{
		print_stderr(where);
		print_stderr(": ");
	}
	
	if(who!=NULL){
		print_stderr(who);
		print_stderr(": ");
	}

	print_stderr(what);
	print_stderr("\n");
}

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

// A DEBEUGER: NE PAS ENCORE UTILISE	
void printss(char* s, char* arg) {
	char ss [strlen(s)];
	char buf [strlen(s)+strlen(arg)-2];
	char a;
	strcpy(ss, s);

	if(strchr(ss, '%') !=NULL) {
		strtok(ss, "%");
		char* reste = strtok(NULL, "");
		a = reste[0];
		if(a == 's') {
			strcpy(buf, ss);
			strcat(buf,arg);
			strcat(buf, &reste[1]);
			prints(buf);
		}
	}
	else 
		prints("\033[1;31mIl n'y a pas d'argument '%s' dans printss()\n\033[0m");
}

void printsss(char* s1, char* arg, char* s2) {
	prints(s1);
	prints(arg);
	prints(s2);
}


// Impression d'un int
void printd(int s) {
	int n = strlen_int(s);
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

