#ifndef SIPP_FILTER_CCMP
#define SIPP_FILTER_CCMP

#include <sipp.h>

typedef struct 
{
    float *ccm;      //ptr to 3x3 Color correction Matrix
}CcmPParam;

void SVU_SYM(svuCcmP)(SippFilter *fptr, int svuNo, int runNo);

#endif
