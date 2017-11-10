#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "core/clock.h"


/************* CLOCK Interface ***************/

#define NSEC_PER_SEC  ((uint64_t)1000000000)
#define NSEC_PER_MSEC ((uint64_t)1000000)
#define NSEC_PER_USEC ((uint64_t)1000)

uint64_t steady_clock_now(void) {
   struct timespec time;
   clock_gettime(CLOCK_REALTIME,&time);
//   uint64_t now = time.tv_nsec;
//   uint64_t now = time.tv_sec * NSEC_PER_SEC;
   return (uint64_t)time.tv_nsec + (uint64_t)time.tv_sec * NSEC_PER_SEC;
//   return now;
}
void microwait(long microsec)
{
  struct timespec time;
  uint64_t nsec = microsec * NSEC_PER_MSEC;
  time.tv_sec =  nsec / NSEC_PER_SEC;
  time.tv_nsec = nsec % NSEC_PER_SEC;
  nanosleep(&time, NULL);
}
