#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <curses.h>
#include <signal.h>

#define HI 5
#define LOW 3

void* createmole(void* id);
void* listenUserInput();
void hiding(int sleepTime, int myId);
void visible(int upTime, int myId);
void printGameboard();
static void finish(int sig);

//Critical section
int* gameboard;
int* playeyKeyPress;
//Mutex lock
sem_t mutex;


int MOLES;
int MAXVISIBLE;
int exiting = 0;

char EMPTYHOLETOP[] = " ____";
char EMPTYHOLEMID[] = "|    |";
char EMPTYHOLEBOT[] = "|____|";

char GOPHERHOLETOP[] = " ____";
char GOPHERHOLEMID[] = "|####|";
char GOPHERHOLEBOT[] = "|####|";

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
  //playerKeyPress = malloc(sizeof(int)*MOLES);
  //Initialize semaphore
  sem_init(&mutex, 0, 1);

  pthread_t listenerThread;
  pthread_t* ids = malloc(sizeof(pthread_t)*MOLES);

  int i;
  for(i = 0; i < MOLES; i++){
    pthread_create(&ids[i], NULL, createmole, (void *)i);
  }

  pthread_create(&listenerThread, NULL, listenUserInput, NULL);

  int num = 0;
  int row, col;
  (void) signal(SIGINT, finish);
  (void) initscr();            /* initialize curses mode */
  getmaxyx(stdscr,row,col);      /* get number of row/col for screen size */
  keypad(stdscr, TRUE);        /* enable keyboard mapping */
  (void) nonl();               /* tell curses not to do NL -> CR/NL on output */
  (void) cbreak();             /* take input chars one at a time, no wait \n */



  for(;;){
    int i;
    for(i = 0; i < MOLES; i++){
        if(gameboard[i] == 1){
          mvprintw(0, (i*6)+2, "%d", i);
          mvprintw(1, (i*6), "%s", GOPHERHOLETOP);
          mvprintw(2, (i*6), "%s", GOPHERHOLEMID);
          mvprintw(3, (i*6), "%s", GOPHERHOLEBOT);
        }
        else{
          mvprintw(0, (i*6)+2, "%d", i);
          mvprintw(1, (i*6), "%s", EMPTYHOLETOP);
          mvprintw(2, (i*6), "%s", EMPTYHOLEMID);
          mvprintw(3, (i*6), "%s", EMPTYHOLEBOT);
        }

    }
    mvprintw(row/2, col/2, "COUNT:%d", i);

    refresh();
    //int c = getch();          /* refresh, accept single keystroke input */
    // attrset(COLOR_PAIR(num % 8));
    // num++;
  }
  finish(0);

  pthread_join(listenerThread, NULL);
  for(i = 0; i < MOLES; i++){
    pthread_join(ids[i], NULL);
  }
  sem_destroy(&mutex);
  free(ids);
  //free(playerKeyPress);
  free(gameboard);
  return 0;
}
static void finish(int sig){
  endwin();
}
void* listenUserInput(){
  //printf("Listener created...\n");
  //USER INPUT
  while(1){
    int c = getch();
  }


  // char * input = malloc(sizeof(char)*10);
  // while(strncmp(input, "quit", 10) < 0){
  //   fgets(input, 10, stdin);
  //   sleep(2);
    //printf("INPUT: %s\n", input);

  //}
  //Cleanup
  //free(input);
  exiting = 1;
  return NULL;
}
void* createmole(void* id){
  long myId = (long)id;
  //printf("Mole %d created...\n", (int)myId);
  while(1){
    if(exiting == 1){
      pthread_exit(0);
    }
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

  //printGameboard();
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

  //printGameboard();
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
