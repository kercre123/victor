#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2010 SignalEssence; All Rights Reserved

  Module Name  - se_def.h

  Description:

      Include file for Signal Essence Discrete Fourier Transform functions.

  History:

  Jul15,2010  hjm  first created
  Aug17,2010  ryu  add fixed-point implementation

  Machine/Compiler: ANSI C
***************************************************************************/

#ifndef __se_dft_public_h
#define __se_dft_public_h

#include "se_types.h"
#include "se_platform.h"

typedef enum {
    SE_DFT_DEFAULT = 0,
    SE_DFT_FFTPACK,
    SE_DFT_FFTW,
    SE_DFT_FXP,
    SE_DFT_PFFFT,
    SE_DFT_QF,
    SE_DFT_LAST
} SeDftLib_t;

//
// call this function to specify a DFT implementation,
void SeDftSetApi(SeDftLib_t dftLib);
#endif //ifndef
#ifdef __cplusplus
}
#endif

