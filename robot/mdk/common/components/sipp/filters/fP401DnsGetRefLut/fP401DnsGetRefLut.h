#include <sipp.h>

typedef struct
{
    UInt8 *lut;
    int maxVal;
}
FP401DnsGetRefLutParam;

void SVU_SYM(svufP401DnsGetRefLut)(SippFilter *fptr, int svuNo, int runNo); 
