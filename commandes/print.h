#ifndef PRINT_H
#define PRINT_H


// Impression d'un char*
void prints(char* s);

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
void printd(int s);

// Impression d'un char
void printc(char s);

// Impression d'un int en base octal
void printo(int s);

#endif
