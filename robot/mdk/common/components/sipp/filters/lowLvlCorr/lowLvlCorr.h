#ifndef SIPP_FILTER_LOW_LEVEL_CORRECTIONS_H
#define SIPP_FILTER_LOW_LEVEL_CORRECTIONS_H

#include <sipp.h>

typedef struct 
{
    UInt8 blackLevel; //
    float alphaBadPixel;
}LowLvlCorrParam;

void SVU_SYM(svuLowLvlCorr)(SippFilter *fptr, int svuNo, int runNo);

#endif
