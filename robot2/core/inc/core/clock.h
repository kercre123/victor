#ifndef CORE_CLOCK_H
#define CORE_CLOCK_H

/************* CLOCK Interface ***************/



uint64_t steady_clock_now(void);
void microwait(long microsec);

#endif//CORE_CLOCK_H
