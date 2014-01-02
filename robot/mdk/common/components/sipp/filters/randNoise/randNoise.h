#include <sipp.h>

typedef struct
{
    float strength;
}
RandNoiseParam;

void SVU_SYM(svuGenNoise)(SippFilter *fptr, int svuNo, int runNo); 
