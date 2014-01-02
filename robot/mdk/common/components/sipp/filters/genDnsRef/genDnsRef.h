#include <sipp.h>

typedef struct
{
    UInt8   *lutGamma;
    UInt8   *lutDist;
    int      shift;
}
YDnsRefParam;

void SVU_SYM(svuGenDnsRef)(SippFilter *fptr, int svuNo, int runNo); 
