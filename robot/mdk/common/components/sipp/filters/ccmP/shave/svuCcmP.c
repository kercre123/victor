#include <sipp.h>
#include <sippMacros.h>
#include <filters/ccmP/ccmP.h>

#define clampU12(x) (x < 0 ? 0 : x > 4095 ? 4095 : x)

//##########################################################################################
void svuCcmP(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32         x;
    CcmPParam *param = (CcmPParam *)fptr->params;
    float          *ccm   = param->ccm;


  //Input pointers
    UInt16  *inCR = (UInt16 *)WRESOLVE(fptr->dbLinesIn[0][runNo&1][0], svuNo);
    UInt16  *inCG = inCR + fptr->parents[0]->planeStride;
    UInt16  *inCB = inCG + fptr->parents[0]->planeStride;
  //Output pointers
    UInt16  *outR = (UInt16 *)WRESOLVE(fptr->dbLineOut   [runNo&1]   , svuNo);
    UInt16  *outG = outR + fptr->planeStride; 
    UInt16  *outB = outG + fptr->planeStride; 


    for(x=0; x<fptr->sliceWidth; x++)
    {
      //Color Correction Matrix
		outR[x] = (UInt16)(clampU12(inCR[x] * ccm[0]  +  inCG[x] * ccm[1]  +  inCB[x] * ccm[2]) + 0.5f);
        outG[x] = (UInt16)(clampU12(inCR[x] * ccm[3]  +  inCG[x] * ccm[4]  +  inCB[x] * ccm[5]) + 0.5f);
        outB[x] = (UInt16)(clampU12(inCR[x] * ccm[6]  +  inCG[x] * ccm[7]  +  inCB[x] * ccm[8]) + 0.5f);
    }
}
