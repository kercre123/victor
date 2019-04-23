#ifdef __cplusplus
extern "C" {
#endif

#include "se_types.h"

/***************************************************************************
   (C) Copyright 2012 SignalEssence; All Rights Reserved

    Module Name: sig_sinusoid

    Author: Robert Yu

    Description:
    sinusoid signal generator

    History:
    2012-06-21  ryu

    Machine/Compiler:
    (ANSI C)
**************************************************************************/
#ifndef SIG_SINUSOID_H_
#define SIG_SINUSOID_H_


typedef enum
{
    SIG_FIRST_DONT_USE,
    SIG_PI,
    SIG_PI_OVER_TWO,
    SIG_PI_OVER_FOUR,
    SIG_PI_OVER_EIGHT,
    SIG_LAST
} SigSinFreq_t;

typedef struct
{
    SigSinFreq_t freq;
    int phase;
    int phaseIncrement;
} SigSin_t;

void SigSinInit(SigSin_t *pSigSin,
                SigSinFreq_t freq);

void SigSinGenerate_i16(SigSin_t *pSigSin,
                      int16 *pOut,
                       uint16 numSamps,
                       uint16 rshifts);


#endif // SIG_SINUSOID
#ifdef __cplusplus
}
#endif
