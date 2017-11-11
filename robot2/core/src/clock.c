#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "core/clock.h"


#include <stdio.h>

/************* CLOCK Interface ***************/


uint64_t steady_clock_now(void) {
   struct timespec time;
   clock_gettime(CLOCK_MONOTONIC,&time);
  return (uint64_t)time.tv_nsec + (uint64_t)time.tv_sec * NSEC_PER_SEC;
}

void microwait(long microsec)
{
  usleep(microsec);
}
