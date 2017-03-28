#include <stdio.h>


int main(int argc, char** argv){

  if(argc < 2){
    perror("No arguments supplied\n");
    perror("./game.c <number>")
    exit(1)
  }

  printf("Number:%d\n", argv[1]);

  return 0;
}
