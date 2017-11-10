#ifndef CORE_CLOCK_H
#define CORE_CLOCK_H

/************* CLOCK Interface ***************/

#define NSEC_PER_SEC  1000000000
#define NSEC_PER_MSEC 1000000
#define NSEC_PER_USEC 1000


uint64_t steady_clock_now(void);
void microwait(long microsec);

#endif//CORE_CLOCK_H
