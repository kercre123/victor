#include <sipp.h>
#include <sippMacros.h>
#include <filters/genLuma/genLuma.h>

//##########################################################################################
void svuGenLuma(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 i;

  //Input RGB lines 
    //UInt8 *inR = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    UInt8 *inR = (UInt8*)getInPtr(fptr, 0, 0, 0);
    UInt8 *inG = (UInt8*)getInPtr(fptr, 0, 0, 1);
    UInt8 *inB = (UInt8*)getInPtr(fptr, 0, 0, 2);

  //Output U8f
    //UInt8 *out  = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1],     svuNo);
    UInt8 *out  = (UInt8 *)getOutPtr(fptr, 0);

  //The processing
    for(i=0; i<fptr->sliceWidth; i++) {
      out[i] = (inR[i]*0.25f + inG[i]*0.50f + inB[i]*0.25f);
    }
}     
