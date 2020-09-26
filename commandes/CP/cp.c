#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "tar.h"


int main(int argc, char *argv[]){

  
  if (argc < 4 || argc > 4) {
    printf("Need 3 argument of type -catCP name.tar pathname/of/file name_of_copy-");
    return -1;
  }
  
	  
  int n;
  
  char tmp;
  struct posix_header hd;
  
  int fd = open(argv[1], O_RDONLY);
  
  if (fd < 0) return -1;
  
  if(fd == -1){
    printf("argument 1 need to be a .tar\n");
    return -1;
  }
  
  unsigned int size;


  // THIS LOOP ALLOWS US TO LOOK FOR THE HEADER CORRESPONDING TO THE FILE WE WANT TO
  // FIND IN THE TAR


  do{
    // READING AN HEADER

    int rdcount=read(fd,&hd,BLOCKSIZE);

    //ERROR MANAGMENT

    if(rdcount<0){

      perror("reading tar file");
      close(fd);
      return -1;
    }

    //IF WE REACHED THE END OF THE TAR WITHOUT FINDING THE GOOD HEADER

    if((hd.name[0]=='\0')){

      printf("Error, file not found int the .tar\n");
      return -1;
    }

    //READONG THE SIZE OF THE FILE CORRESPONDING TO THE CURRENT HEADER

			
    sscanf(hd.size, "%o",&size);

    //IF WE FOUND THE RIGHT HEADER, WE GET OUT OF THE LOOP

    if(strcmp(hd.name,argv[2])==0){

      break;

    }

    //OTHERWISE WE GET TO THE NEXT HEADER

    lseek(fd,((size+ BLOCKSIZE - 1) >> BLOCKBITS)*BLOCKSIZE,SEEK_CUR);


  }while(strcmp(hd.name,argv[2])!=0);	

  int fr = open(argv[2], O_RDONLY);
  
  int ft = open ( argv[3], O_WRONLY | O_CREAT , S_IRUSR | S_IWUSR);
    
  while((n = read(fr, &tmp, 1)) > 0){    
    write(ft , &tmp, 1);
  }

  
  close (ft);
  close (fr);
  close (fd);
  return 0;


  }
     
  
	         
  
  

