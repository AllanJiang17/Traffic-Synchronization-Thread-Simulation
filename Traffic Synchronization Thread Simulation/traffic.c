#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "uthread.h"
#include "uthread_mutex_cond.h"

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__)
#else
#define VERBOSE_PRINT(S, ...) ((void) 0) // do nothing
#endif

#define MAX_OCCUPANCY  3
#define NUM_ITERATIONS 100
#define NUM_CARS       20

// These times determine the number of times yield is called when in
// the street, or when waiting before crossing again.
#define CROSSING_TIME             NUM_CARS
#define WAIT_TIME_BETWEEN_CROSSES NUM_CARS

/**
 * You might find these declarations useful.
 */
enum Direction {EAST = 0, WEST = 1};
const static enum Direction oppositeEnd [] = {WEST, EAST};

struct Street {
  uthread_mutex_t mx;
  uthread_cond_t empty;
  uthread_cond_t notFull;
  uthread_cond_t otherDirection;
  enum Direction direction;

  int countCars;
} Street;

void initializeStreet(void) {
  Street.mx = uthread_mutex_create();
  Street.empty = uthread_cond_create(Street.mx);
  Street.notFull = uthread_cond_create(Street.mx);
  Street.otherDirection = uthread_cond_create(Street.mx); 
  Street.countCars = 0;
}

#define WAITING_HISTOGRAM_SIZE (NUM_ITERATIONS * NUM_CARS)
int             entryTicker;                                          // incremented with each entry
int             waitingHistogram [WAITING_HISTOGRAM_SIZE];
int             waitingHistogramOverflow;
uthread_mutex_t waitingHistogramLock;
int             occupancyHistogram [2] [MAX_OCCUPANCY + 1];

void leaveStreet(void) {
  uthread_mutex_lock(Street.mx);
  assert(Street.countCars <= 3);
  Street.countCars--;
  //printf("car left\n");
  uthread_cond_signal(Street.notFull);
  if (Street.countCars == 0) {
    uthread_cond_broadcast(Street.empty);
  }
  uthread_mutex_unlock(Street.mx);
}

void enterStreet (enum Direction g) {
   Street.countCars++;
   assert(Street.direction == g);
   assert(Street.countCars <= 3);
   occupancyHistogram[g][Street.countCars]++;
   uthread_mutex_unlock(Street.mx);
   entryTicker++;
   for (int i = 0; i < NUM_CARS; i++) {
     uthread_yield();
   }
}

void recordWaitingTime (int waitingTime) {
  uthread_mutex_lock (waitingHistogramLock);
  if (waitingTime < WAITING_HISTOGRAM_SIZE)
    waitingHistogram [waitingTime] ++;
  else
    waitingHistogramOverflow ++;
  uthread_mutex_unlock (waitingHistogramLock);
}

//
// TODO
// You will probably need to create some additional procedures etc.
//

void * goingEastFlagger(void * arg) {
  for (int i = 0; i < NUM_ITERATIONS; i++) {
uthread_mutex_lock(Street.mx);
   int start = entryTicker;
  if (Street.countCars == 0) {
    Street.direction = EAST;
    uthread_cond_signal(Street.otherDirection);
    
   // printf("car going east, 0 cars\n");
    enterStreet(EAST);
    recordWaitingTime(entryTicker - start);
  } else if (Street.direction == EAST) {
      while (Street.countCars >= 3) {
        uthread_cond_wait(Street.notFull);
      }
       while (Street.direction != EAST) {
        uthread_cond_wait(Street.otherDirection);
      }
     // printf("car going east, some cars\n");
      assert(Street.direction == EAST);
      
      enterStreet(EAST);
      recordWaitingTime(entryTicker - start);
  } else {
    //   printf("waiting for road to go east\n");
       while(Street.countCars != 0) {
         uthread_cond_wait(Street.empty);
       }
       uthread_cond_signal(Street.otherDirection);
       Street.direction = EAST;
       assert(Street.countCars == 0);
       enterStreet(EAST);
       recordWaitingTime(entryTicker - start);
      }
      leaveStreet();
      for (int i = 0; i < NUM_CARS; i++) {
         uthread_yield();
   }
  }
     return NULL;
}

void * goingWestFlagger(void * arg){
for (int i = 0; i < NUM_ITERATIONS; i++) {

  uthread_mutex_lock(Street.mx);
  int start = entryTicker;
  if (Street.countCars == 0) {
    Street.direction = WEST;
    uthread_cond_signal(Street.otherDirection);
  //  printf("car going west, 0 cars\n");
    enterStreet(WEST);
    recordWaitingTime(entryTicker - start);
  } else if (Street.direction == WEST) {
      while (Street.countCars >= 3) {
        uthread_cond_wait(Street.notFull);
      }
      while (Street.direction != WEST) {
        uthread_cond_wait(Street.otherDirection);
      }
      assert(Street.direction == WEST);
     // printf("car going west, some cars\n");
      
      enterStreet(WEST);
      recordWaitingTime(entryTicker - start);
  } else {
      //  printf("waiting for road to go west\n");
        while(Street.countCars != 0) {
         uthread_cond_wait(Street.empty);
       }
       uthread_cond_signal(Street.otherDirection);
       Street.direction = WEST;
      
       enterStreet(WEST);
       recordWaitingTime(entryTicker - start);
      }
      leaveStreet();
      for (int i = 0; i < NUM_CARS; i++) {
         uthread_yield();
   }
}
    
     return NULL;
  }
  


int main (int argc, char** argv) {
  uthread_init(8);

  waitingHistogramLock = uthread_mutex_create();
  
  initializeStreet();
  uthread_t pt [NUM_CARS];
  
  for (int i = 0; i < NUM_CARS; i++) {
    int rand = random() % 2;
    if (rand == 0) {
      pt[i] = uthread_create(goingEastFlagger, NULL);
    } else if (rand == 1) {
      pt[i] = uthread_create(goingWestFlagger, NULL);
    }
  }


  for (int i = 0; i < NUM_CARS; i++) {
    uthread_join(pt[i], NULL);
  }
  
  printf ("Times with 1 car  going east: %d\n", occupancyHistogram [EAST] [1]);
  printf ("Times with 2 cars going east: %d\n", occupancyHistogram [EAST] [2]);
  printf ("Times with 3 cars going east: %d\n", occupancyHistogram [EAST] [3]);
  printf ("Times with 1 car  going west: %d\n", occupancyHistogram [WEST] [1]);
  printf ("Times with 2 cars going west: %d\n", occupancyHistogram [WEST] [2]);
  printf ("Times with 3 cars going west: %d\n", occupancyHistogram [WEST] [3]);
  
  printf ("Waiting Histogram\n");
  for (int i=0; i < WAITING_HISTOGRAM_SIZE; i++)
    if (waitingHistogram [i])
      printf ("  Cars waited for           %4d car%s to enter: %4d time(s)\n",
	      i, i==1 ? " " : "s", waitingHistogram [i]);
  if (waitingHistogramOverflow)
    printf ("  Cars waited for more than %4d cars to enter: %4d time(s)\n",
	    WAITING_HISTOGRAM_SIZE, waitingHistogramOverflow);
}