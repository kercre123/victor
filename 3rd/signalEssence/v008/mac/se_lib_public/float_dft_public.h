#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************
 (C) Copyright 2012 SignalEssence; All Rights Reserved

  Module Name  - float_dft.h

  Description:
    General API for floating point DFT (floating point ins and outs)

  History:    
   2012-06-27  ryu   first created

  Machine/Compiler: ANSI C
***************************************************************************/

#ifndef _FLOAT_DFT_PUB_H
#define _FLOAT_DFT_PUB_H

#include "mmglobalcodes.h"
#include "se_types.h"
#include "se_platform.h"

//
// list of all supported float dft implementations
typedef enum {
    FLOAT_DFT_DEFAULT = 0,
    FLOAT_DFT_FFTPACK,
    FLOAT_DFT_LIBAV,
    FLOAT_DFT_PFFFT,
    FLOAT_DFT_FFTW,
    FLOAT_DFT_C67,
    FLOAT_DFT_LAST
} FloatDftLib_t;

//
// call this function to specify a DFT implementation,
void FloatDftSetApi(FloatDftLib_t dftLib);

#endif

#ifdef __cplusplus
}
#endif

