#ifndef SIPP_FILTER_AVG_H
#define SIPP_FILTER_AVG_H

#include <sipp.h>

typedef struct 
{
    float *ccm;      //ptr to 3x3 Color correction Matrix
    UInt8 *rangeLut; //range lut
}ChromaBlkParam;

void SVU_SYM(svuChromaBlock)(SippFilter *fptr, int svuNo, int runNo);

#endif
