#include <sipp.h>

typedef struct
{
    UInt8 *lut;
    int   shift;
}
YDnsRefPParam;

void SVU_SYM(svuGenDnsRefP)(SippFilter *fptr, int svuNo, int runNo); 
