#include <sipp.h>

//##########################################################################################
void svuAvg(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 x;

    UInt8  *out = (UInt8 *)WRESOLVE(       fptr->dbLineOut   [runNo&1]   , svuNo);
    UInt8  *inA = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    UInt8  *inB = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[1][runNo&1][1], svuNo);
    
    for(x=0; x<fptr->sliceWidth; x++)
    {
        out[x] = ((int)inA[x] + (int)inB[x] + 1)>>1;
    }
}
