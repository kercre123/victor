/** 
  Tap points
  
  Tap points are places in the code where we can
  process/analyze/touch the signal.  The locations of tap points are
  pre-defined, but the actual callback can be set dynamically.


  To use:
  define a callback function:

TapPointResult_t  MyCallback(void *pInOut, 
                             int32 numChannels,
                             int32 numElemPerChan, 
                             size_t sizeOfElem,
                             void *arg)
{
       ...
    callback actions
       ...
}

  Then register the callback with the desired tappoint.  Tappoints are
  listed in MMfxPubObj and are generally called in mmfx.c

TapPointSet(&(pMMFxPubObj->TapPreSpatialFilter),
            MyCallback,
            NULL);

   The last argument can be used to pass a pointer (or other arg) to the
   callback function
*/ 

#ifdef __cplusplus
extern "C" {
#endif
#ifndef _TAP_POINT_H
#define _TAP_POINT_H
#include "se_types.h"
#include <stddef.h>

typedef enum {
    TP_FIRST_DONT_USE,
    TP_CONTINUE,      // Continue processing in the calling function
    TP_ABORT,         // Abort processing in calling function.  This is
                      // *NOT* an error condition, rather it just tells
                      // the calling function to not do any more
                      // processing.


    // SPECIALTY TAP POINT RETURN CODES HERE
    //
    TP_SERCV_JUMP_TO_CROSSOVER, // A specfial return code that tells the SE_RCV path to jump directly to the crossover and skip other processing.
    TP_SERCV_JUMP_OVER_CROSSOVER,    // A specfial return code that tells the SE_RCV path to jump directly PAST the crossover and skip other processing.
    TP_LAST
} TapPointResult_t;

typedef TapPointResult_t  (*TapPointFunc_t)(void *pInOut, int32 numChannels, int32 numElemPerChan, size_t sizeOfElem, void *arg);

typedef struct {
    TapPointFunc_t func;
    void          *arg;
} TapPoint_t;

void TapPointSet  (TapPoint_t *tapPoint, TapPointFunc_t function, void *arg);
int  TapPointIsSet(TapPoint_t *tapPoint);
void TapPointClear(TapPoint_t *tapPoint);
TapPointResult_t TapPointCall(TapPoint_t tapPoint, void *pInOut, int32 numChannels, int32 numElemPerChan, size_t sizeOfElem);

#endif
#ifdef __cplusplus
}
#endif
