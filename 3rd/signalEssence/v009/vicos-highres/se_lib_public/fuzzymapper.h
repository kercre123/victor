#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2010 Signal Essence; All Rights Reserved

  Module Name  - fuzzymapper.c

  Author: Hugh McLaughlin

  Description:

     Fuzzy mapper is a function that takes in an input value and a threshold
     and maps the ratio to a fuzzy logic value 0.0 to 1.0.  

  History:    hjm - Hugh McLaughlin 

  10-05-10    hjm  created
  2011-11-28  ryu  moved FUZZY_NUM_CURVE_PNTS to se_lib_public/mmglobalcodes.h

  Machine/Compiler: ANSI C
--------------------------------------------------------------------------*/

#ifndef __fuzzymapper_h
#define __fuzzymapper_h

/* Function Prototypes */
#include "se_types.h"
#include "mmglobalcodes.h"

float32 MapRatioToFuzzy( const float32 threshFactors[FUZZY_NUM_CURVE_PNTS], 
                         const float32 fuzzyPts[FUZZY_NUM_CURVE_PNTS],
                         float32 input, 
                         float32 threshold );




#endif
#ifdef __cplusplus
}
#endif

