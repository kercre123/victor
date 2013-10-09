/**
 * File: assert.h
 *
 * Description:  Assert system for debugging on vehicle
 * 
 * Asserts are lightweight - if the alternative is a nasty crash, leave the assert in shipping builds!
 * The vehicle will reset and radio home the assert address in a crash report
 * Use the .map file to map the assert address back to the offending line of code
 * Asserts are also stored in stats (see stats.h)
 *
 * Copyright: Anki, Inc. 2008
 **/

#ifndef ASSERT__H_
#define ASSERT__H_

#ifdef __COZMO_CPU

#else
#include <stdio.h>
#define ASSERT(condition) { if (!(condition)) printf("ASSERT: %s\n", #condition);}
#endif


#define assert(condition) ASSERT(condition)

#endif // ASSERT__H_
