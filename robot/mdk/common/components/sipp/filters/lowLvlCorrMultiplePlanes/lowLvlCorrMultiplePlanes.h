#ifndef SIPP_FILTER_LOW_LEVEL_CORRECTIONS_MULTIPLE_PLANES_H
#define SIPP_FILTER_LOW_LEVEL_CORRECTIONS_MULTIPLE_PLANES_H

#include <sipp.h>

typedef struct 
{
    UInt8 blackLevel; //
    float alphaBadPixel;
}LowLvlCorrNPlParam;

void SVU_SYM(svulowLvlCorrMultiplePlanes)(SippFilter *fptr, int svuNo, int runNo);

#endif
