#include <sipp.h>

typedef struct
{
    float strength;
}
FP406RandNoiseParam;

void SVU_SYM(svufP406RandNoise)(SippFilter *fptr, int svuNo, int runNo); 
