#include <sipp.h>

typedef struct
{
    UInt8 *lut;
}
YDnsRefLut10bppParam;

void SVU_SYM(svuLutP10BppU16inU8out)(SippFilter *fptr, int svuNo, int runNo); 
