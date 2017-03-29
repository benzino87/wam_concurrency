#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

#define HI 10
#define LOW 3

void* createmole(void* id);
void* listenUserInput();
void hiding(int sleepTime, int myId);
void visible(int upTime, int myId);
void printGameboard();

//Critical section
int* gameboard;
//Mutex lock
sem_t mutex;

int MOLES;
int MAXVISIBLE;
int listenerStatus = 0;

int main(int argc, char** argv){

  if(argc != 3){
    perror("Too few or too many arguments supplied:");
    printf("./executable <number>\n");
    exit(1);
  }

  MOLES = atoi(argv[1]);
  MAXVISIBLE = atoi(argv[2]);

  if(MOLES <= 0 || MOLES > 10){
    perror("Number must be a range from 1 - 10:");
    exit(1);
  }
  if(MAXVISIBLE <= 0 || MAXVISIBLE > MOLES){
    perror("Number must be a range from 1 - MOLES:");
    exit(1);
  }

  //Initialize the critical section
  gameboard = malloc(sizeof(int)*MOLES);
  //Initialize semaphore
  sem_init(&mutex, 0, 1);

  pthread_t listenerThread;
  pthread_t* ids = malloc(sizeof(pthread_t)*MOLES);

  int i;
  for(i = 0; i < MOLES; i++){
    pthread_create(&ids[i], NULL, createmole, (void *)i);
  }

  pthread_create(&listenerThread, NULL, listenUserInput, NULL);

  while(listenerStatus != -1){
      //busy wait
  }

  pthread_join(listenerThread, NULL);
  for(i = 0; i < MOLES; i++){
    pthread_join(ids[i], NULL);
  }
  sem_destroy(&mutex);
  free(ids);

  free(gameboard);
  return 0;
}

void* listenUserInput(){
  //USER INPUT
  char * input = malloc(sizeof(char)*10);
  while(strncmp(input, "quit", 10) < 0){
    read(STDIN_FILENO, input, sizeof(char)*10);
    //fgets(input, 10, stdin);
  }
  //Cleanup
  if(strncmp(input, "quit", 10) == 1){
    free(input);
    listenerStatus = -1;
    printf("HERE!!!\n");
  }
  return NULL;
}
void* createmole(void* id){
  long myId = (long)id;
  while(1){
    hiding(rand() % (HI - LOW), myId);
    visible(rand() % (HI - LOW), myId);
  }
  return NULL;
}

//Request access, alter mutex
void hiding(int sleepTime, int myId){

  sem_wait(&mutex);
  gameboard[myId] = 0;
  sem_post(&mutex);

  printGameboard();
  sleep(sleepTime);
}

//Request access, alter mutex
void visible(int upTime, int myId){
  int i;
  int count = 0;

  sem_wait(&mutex);
  for(i = 0; i < MOLES; i++){
    if(gameboard[i] == 1){
      count++;
    }
  }
  if(count < MAXVISIBLE){
    gameboard[myId] = 1;
  }
  sem_post(&mutex);

  printGameboard();
  sleep(upTime);
}

//Testing purposes
void printGameboard(){
  int i;
  for(i = 0; i < MOLES; i++){
    if(i == 0){
      printf("[%d,", gameboard[i]);
    }
    else if(i == MOLES-1){
      printf("%d]\n", gameboard[i]);
    }
    else{
      printf("%d,", gameboard[i]);
    }
  }
}
