#include <sipp.h>

typedef struct
{
    UInt8 *lut;
    int   shift;
}
FP402DnsGetRefParam;

void SVU_SYM(svufP402DnsGetRef)(SippFilter *fptr, int svuNo, int runNo); 
