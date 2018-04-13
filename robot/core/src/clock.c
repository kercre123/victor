#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "core/clock.h"


/************* CLOCK Interface ***************/


uint64_t steady_clock_now(void) {
   struct timespec time;
   clock_gettime(CLOCK_MONOTONIC,&time);
   return time.tv_nsec + time.tv_sec * NSEC_PER_SEC;
}
void microwait(long microsec)
{
  struct timespec time;
  uint64_t nsec = microsec * NSEC_PER_MSEC;
  time.tv_sec =  nsec / NSEC_PER_SEC;
  time.tv_nsec = nsec % NSEC_PER_SEC;
  nanosleep(&time, NULL);
}

void milliwait(long millisec)
{
  usleep(millisec * 1000);
}
