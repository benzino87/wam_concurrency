/**
 * Author: Jason Bensel
 * Description: Multithreaded, mutex locked Wack-a-mole game
 */
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

//Sleep and awake times for moles
#define HI 5
#define LOW 3

//Thread to handle mole sleep and up times
void* createmole(void* id);
//Listen for player input keystrokes
void* listenUserInput();
//Alters the mutex locked array to indicate mole for this thread is sleeping
void hiding(int sleepTime, int myId);
//Alters the mutex locked array to indicate mole for this thread is awake
void visible(int upTime, int myId);

//Critical section
int* gameboard;

//Mutex locks
//Gameboard lock
sem_t mutex;
//Screen lock
sem_t screenMutex;

//Total moles
int MOLES;
//Max that can be awake at a given time
int MAXVISIBLE;
//Player score
int HITS = 0;
//Player misses
int MISSES = 0;
//Checked by threads to indicate game is over
int exiting = 0;


// Pretty title
char TITLETOP1[] = " _      ____            __                           __";
char TITLETOP2[] = "| | /| / / /  ___ _____/ /__    ____     ____  ___  / /__";
char TITLETOP3[] = "| |/ |/ / _ :/ _ `/ __/  '_/   / _ `/   /  ' :/ _ :/ / -_)";
char TITLETOP4[] = "|__/|__/_//_/:_,_/:__/_/:_:    :_,_/   /_/_/_/:___/_/:__/";


// Empty hole
char EMPTYHOLETOP[] = " ____";
char EMPTYHOLEMID[] = "|    |";
char EMPTYHOLEBOT[] = "|____|";

// Gopher hole
char GOPHERHOLETOP[] = " ____";
char GOPHERHOLEMID[] = "|o  o|";
char GOPHERHOLEBOT[] = "|_''_|";

// Gopher hole
char GOPHERHITTOP[] = " ____";
char GOPHERHITMID[] = "|####|";
char GOPHERHITBOT[] = "|####|";

/**
 * Main method handles all of the game logic. It spawns off a number of threads
 * associated with number of moles. Each mole sleeps for a random amount of time,
 * then becomes awake for a random amount of time. A seperate thread for user
 * input listens for keystrokes, if an associated keystroke matches a mole thread
 * that is awake, the player gets a point. If no mole is associated with keystroke
 * the player gets a miss.
 */
int main(int argc, char** argv){

  //Verify valid arguments supplied
  if(argc != 3){
    perror("Too few or too many arguments supplied:");
    printf("./executable <number of moles> <number displayed>\n");
    exit(1);
  }

  MOLES = atoi(argv[1]);
  MAXVISIBLE = atoi(argv[2]);

  //Verify valid range of moles
  if(MOLES <= 0 || MOLES > 10){
    perror("Number must be a range from 1 - 10:");
    exit(1);
  }

  //Verify valid range of max visible at any given time
  if(MAXVISIBLE <= 0 || MAXVISIBLE > MOLES){
    perror("Number must be a range from 1 - MOLES:");
    exit(1);
  }


  //Initialize the critical section
  gameboard = malloc(sizeof(int)*MOLES);

  //Initialize semaphores
  sem_init(&mutex, 0, 1);
  sem_init(&screenMutex, 0, 1);

  //Create thread ids
  pthread_t listenerThread;
  pthread_t* ids = malloc(sizeof(pthread_t)*MOLES);

  //Spawn all mole threads
  int i;
  for(i = 0; i < MOLES; i++){
    pthread_create(&ids[i], NULL, createmole, (void *)i);
  }

  //Spawn listener thread
  pthread_create(&listenerThread, NULL, listenUserInput, NULL);

  //Initialize attributes for ncurses
  int row, col;
  (void) initscr();            /* initialize curses mode */
  getmaxyx(stdscr,row,col);    /* get number of row/col for screen size */
  (void) cbreak();             /* take input chars one at a time, no wait \n */
  (void) noecho();             /* prevent keystrokes from echoing to screen */

  //Get a start time
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

        if(gameboard[i] == 0){
          mvprintw(5, (i*6)+2, "%d", i);
          mvprintw(6, (i*6), "%s", EMPTYHOLETOP);
          mvprintw(7, (i*6), "%s", EMPTYHOLEMID);
          mvprintw(8, (i*6), "%s", EMPTYHOLEBOT);

        }
        else if(gameboard[i] == 1){
          mvprintw(5, (i*6)+2, "%d", i);
          mvprintw(6, (i*6), "%s", GOPHERHOLETOP);
          mvprintw(7, (i*6), "%s", GOPHERHOLEMID);
          mvprintw(8, (i*6), "%s", GOPHERHOLEBOT);
        }
        else if(gameboard[i] == 2){
          mvprintw(5, (i*6)+2, "%d", i);
          mvprintw(6, (i*6), "%s", GOPHERHITTOP);
          mvprintw(7, (i*6), "%s", GOPHERHITMID);
          mvprintw(8, (i*6), "%s", GOPHERHITBOT);
        }
    }

    //Grab current time to notify interface how much time has passed
    time_t curTime = time(NULL);
    time_t totalTime = 30;

    //Check for listener thread to alter game status
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

    //Time checks for game end
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

    mvprintw(row/2, col/2, "HITS:%d", HITS);
    mvprintw((row/2)+1, col/2, "MISSES:%d", MISSES);
    mvprintw((row/2)+2, col/2, "TIMER:%d", totalTime-(curTime-startTime));
    refresh();
    sem_post(&screenMutex);
  }

  //Clean-up, join all threads on exit flag
  pthread_join(listenerThread, NULL);
  for(i = 0; i < MOLES; i++){
    pthread_join(ids[i], NULL);
  }
  sem_destroy(&mutex);
  sem_destroy(&screenMutex);
  free(ids);
  free(gameboard);

  return 0;
}

/**
 * Manages listning for user input during game session. Reads data from the
 * gameboard array to find a match to the given keystroke. If found, it
 * increments score("HITS") and immediately alters gameboard to indicate a
 * mole has been hit.
 */
void* listenUserInput(){
  while(1){
    char c = getch();          /* refresh, accept single keystroke input */
    int pos = atoi(&c);

    //Wait on main thread to finish writing to screen
    sem_wait(&screenMutex);
    if(gameboard[pos] == 1){
      HITS++;
      //Do not wait on lock to be freed. We do not want this to be qued up with
      //moles
      gameboard[pos] = 2;
    }else{
      MISSES++;
    }
    //Refresh page and release screen lock for main thread to continue
    refresh();
    sem_post(&screenMutex);

    //Win status
    if(HITS == 30){
      exiting = 1;
    }
  }
  return NULL;
}

/**
 * Method that handles all mole logic. Delegates sleep and awake commands to Each
 * mole thread.
 *
 * param: (id) ID given to thread which is associated with it's position on the
 *             gameboard array.
 */
void* createmole(void* id){
  long myId = (long)id;
  while(1){
    if(exiting == 1){
      pthread_exit(0);
    }
    hiding(rand() % (HI - LOW), myId);
    visible(rand() % (HI - LOW), myId);
  }
  return NULL;
}

/**
 * Requests access to gameboard array, and instructs mole at position myId
 * to sleep for the given ammount of time
 *
 * param: (sleepTime) amount of time in which thread sleeps
 * param: (myId) ID of current thread associated with a spot in the gameboard
 *               array.
 */
void hiding(int sleepTime, int myId){

  sem_wait(&mutex);
  gameboard[myId] = 0;
  sem_post(&mutex);
  sleep(sleepTime);
}

/**
 * Requests access to gameboard array, and instructs mole at position myId
 * to become visible for the given ammount of time
 *
 * param: (sleepTime) amount of time in which thread is awake
 * param: (myId) ID of current thread associated with a spot in the gameboard
 *               array.
 */
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
