#ifndef CORE_CLOCK_H
#define CORE_CLOCK_H

#include <stdint.h>

/************* CLOCK Interface ***************/

#define NSEC_PER_SEC  ((uint64_t)1000000000)
#define NSEC_PER_MSEC ((uint64_t)1000000)
#define NSEC_PER_USEC (1000)


uint64_t steady_clock_now(void);
void microwait(long microsec);
void milliwait(long millisec);

#endif//CORE_CLOCK_H
