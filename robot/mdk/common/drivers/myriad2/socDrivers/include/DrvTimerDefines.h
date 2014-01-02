///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by Timer Driver
/// 
#ifndef DRV_TIMER_DEF_H
#define DRV_TIMER_DEF_H 

#include "mv_types.h"

// API ERRORS
#define     ERR_NO_FREE_TIMER                           (-1)
#define     ERR_SINGLE_SHOT_ONLY_LARGE_TIMER            (-2)
#define     ERR_TIM_ALREADY_INTIALISED                  (-3)

#define REPEAT_FOREVER              (0)
#define US_IN_1_MS                  (1000)
#define US_IN_1_SECOND              (1000 * 1000)

#define DEFAULT_TIMER_PRIORITY      (3)

#define NUM_TIMERS           (8)

// 1: Defines
// ----------------------------------------------------------------------------

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

typedef u32 (*tyTimerCallback)(u32 timerNumber, u32 param2); 
typedef u64 tyTimeStamp;

typedef struct 
{
    union 
    {
        u64 dword;
        u32 word[2];
    };
} tyDwordUnion;

               
// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // DRV_TIMER_DEF_H

