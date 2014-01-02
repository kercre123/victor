#include <sipp.h>
#include <sippMacros.h>
#include <filters/contrast/contrast.h>

//##########################################################################################
void svuContrast(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 i;
    ContrastParam *param = (ContrastParam *)fptr->params;

  //Input RGB lines 
    UInt8 *inR  = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    UInt8 *inG  = inR + fptr->parents[0]->planeStride;
    UInt8 *inB  = inG + fptr->parents[0]->planeStride;

  //Output RGB lines
    UInt8 *outR = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1],       svuNo);
    UInt8 *outG = outR + fptr->planeStride;
    UInt8 *outB = outG + fptr->planeStride;

  //Contrast
    for(i=0; i<fptr->sliceWidth; i++)
    {
      #if 1
        //img = double(img - idxLow) * scale;
        outR[i] = clampU8((inR[i] - param->idxLow) * param->scale);
        outG[i] = clampU8((inG[i] - param->idxLow) * param->scale);
        outB[i] = clampU8((inB[i] - param->idxLow) * param->scale);
      #else //copy-paste for test...
        outR[i] = inR[i];
        outG[i] = inG[i];
        outB[i] = inB[i];
      #endif
    }
} 
