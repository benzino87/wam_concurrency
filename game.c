#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <curses.h>
#include <signal.h>
#include <time.h>

#define HI 5
#define LOW 3

void* createmole(void* id);
void* listenUserInput();
void hiding(int sleepTime, int myId);
void visible(int upTime, int myId);

//Critical section
int* gameboard;
int* playeyKeyPress;
//Mutex lock
sem_t mutex;
sem_t screenMutex;

int MOLES;
int MAXVISIBLE;
int HITS = 0;
int MISSES = 0;
int exiting = 0;


char TITLETOP1[] = " _      ____            __                           __";
char TITLETOP2[] = "| | /| / / /  ___ _____/ /__    ____     ____  ___  / /__";
char TITLETOP3[] = "| |/ |/ / _ :/ _ `/ __/  '_/   / _ `/   /  ' :/ _ :/ / -_)";
char TITLETOP4[] = "|__/|__/_//_/:_,_/:__/_/:_:    :_,_/   /_/_/_/:___/_/:__/";


char EMPTYHOLETOP[] = " ____";
char EMPTYHOLEMID[] = "|    |";
char EMPTYHOLEBOT[] = "|____|";

char GOPHERHOLETOP[] = " ____";
char GOPHERHOLEMID[] = "|o  o|";
char GOPHERHOLEBOT[] = "|_''_|";

int main(int argc, char** argv){

  if(argc != 3){
    perror("Too few or too many arguments supplied:");
    printf("./executable <number of moles> <number displayed>\n");
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

  //Initialize semaphores
  sem_init(&mutex, 0, 1);
  sem_init(&screenMutex, 0, 1);

  pthread_t listenerThread;
  pthread_t* ids = malloc(sizeof(pthread_t)*MOLES);

  int i;
  for(i = 0; i < MOLES; i++){
    pthread_create(&ids[i], NULL, createmole, (void *)i);
  }

  pthread_create(&listenerThread, NULL, listenUserInput, NULL);

  int row, col;
  (void) initscr();            /* initialize curses mode */
  getmaxyx(stdscr,row,col);    /* get number of row/col for screen size */
  (void) cbreak();             /* take input chars one at a time, no wait \n */
  (void) noecho();             /* prevent keystrokes from echoing to screen */

  time_t startTime = time(NULL);


  //Iterate through the gameboard and print picture representation of MOLES
  //(up or hiding). This is protected by a semaphore to prevent keystrokes
  //to cause gui printing to change positions
  for(;;){
    int i;
    sem_wait(&screenMutex);
    for(i = 0; i < MOLES; i++){

      mvprintw(0, 0, "%s", TITLETOP1);
      mvprintw(1, 0, "%s", TITLETOP2);
      mvprintw(2, 0, "%s", TITLETOP3);
      mvprintw(3, 0, "%s", TITLETOP4);

        if(gameboard[i] == 1){
          mvprintw(5, (i*6)+2, "%d", i);
          mvprintw(6, (i*6), "%s", GOPHERHOLETOP);
          mvprintw(7, (i*6), "%s", GOPHERHOLEMID);
          mvprintw(8, (i*6), "%s", GOPHERHOLEBOT);
        }

        else{
          mvprintw(5, (i*6)+2, "%d", i);
          mvprintw(6, (i*6), "%s", EMPTYHOLETOP);
          mvprintw(7, (i*6), "%s", EMPTYHOLEMID);
          mvprintw(8, (i*6), "%s", EMPTYHOLEBOT);
        }
    }

    time_t curTime = time(NULL);
    time_t totalTime = 30;

    if(totalTime - (curTime-startTime) == 0){
      clear();
      mvprintw(0, 0, "YOU LOSE!! Hit esc to exit...");
      mvprintw(1, 0, "STATS");
      mvprintw(2, 0, "HITS:%d", HITS);
      mvprintw(3, 0, "MISSES:%d", MISSES);
      char c;
      while(1){
        if((c=getch())==27){
          exiting = 1;
          endwin();
          break;
        }
      }
      break;
    }

    if(exiting == 1){
      clear();
      mvprintw(0, 0, "YOU WIN!! Hit esc to exit...");
      mvprintw(1, 0, "STATS");
      mvprintw(2, 0, "HITS:%d", HITS);
      mvprintw(3, 0, "MISSES:%d", MISSES);
      char c;
      while(1){
        if((c=getch())==27){
          exiting = 1;
          endwin();
          break;
        }
      }
      break;
    }

    mvprintw(row/2, col/2, "HITS:%d", HITS);
    mvprintw((row/2)+1, col/2, "MISSES:%d", MISSES);
    mvprintw((row/2)+2, col/2, "TIMER:%d", totalTime-(curTime-startTime));
    refresh();
    sem_post(&screenMutex);
  }


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


void* listenUserInput(){
  //printf("Listener created...\n");
  while(1){
    char c = getch();          /* refresh, accept single keystroke input */
    int pos = atoi(&c);

    sem_wait(&screenMutex);
    if(gameboard[pos] == 1){
      HITS++;
      //sem_wait(&mutex);
      gameboard[pos] = 0;
      //sem_post(&mutex);
    }else{
      MISSES++;
    }
    refresh();
    sem_post(&screenMutex);
    if(HITS == 30){
      exiting = 1;
    }
  }
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
  sleep(upTime);
}
