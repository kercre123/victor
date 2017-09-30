#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "core/clock.h"


/************* CLOCK Interface ***************/

#define NSEC_PER_SEC  1000000000
#define NSEC_PER_MSEC 1000000
#define NSEC_PER_USEC 1000

uint64_t steady_clock_now(void) {
   struct timespec time;
   clock_gettime(CLOCK_REALTIME,&time);
   return time.tv_nsec + time.tv_sec * NSEC_PER_SEC;
}
void microwait(long microsec)
{
  /* uint64_t start = steady_clock_now(); */
  /* while ((steady_clock_now() - start) < (microsec * NSEC_PER_MSEC)) { */
  /*   ; */
  /* } */
  struct timespec time;
  uint64_t nsec = microsec * NSEC_PER_MSEC;
  time.tv_sec =  nsec / NSEC_PER_SEC;
  time.tv_nsec = nsec % NSEC_PER_SEC;
  nanosleep(&time, NULL);
}
