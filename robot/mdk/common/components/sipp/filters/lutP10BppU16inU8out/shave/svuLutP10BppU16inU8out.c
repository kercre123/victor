#include <sipp.h>
#include <sippMacros.h>
#include <filters/lutP10BppU16inU8out/lutP10BppU16inU8out.h>

#define MAX_LUT_IDX ((1<<10) - 1)

//##########################################################################################
void svuLutP10BppU16inU8out(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 x;

    YDnsRefLut10bppParam *param = (YDnsRefLut10bppParam *)fptr->params;

    //Input Y line
    UInt16 *inY  = (UInt16*)WRESOLVE(fptr->dbLinesIn[0][runNo&1][0], svuNo);

    //Output Y ref
    UInt8 *outY = (UInt8*)WRESOLVE(fptr->dbLineOut[runNo&1],       svuNo);

    for(x=0; x<fptr->sliceWidth; x++)
    {
        outY[x] = param->lut[(inY[x] > MAX_LUT_IDX ? MAX_LUT_IDX : inY[x])];
    }
} 
