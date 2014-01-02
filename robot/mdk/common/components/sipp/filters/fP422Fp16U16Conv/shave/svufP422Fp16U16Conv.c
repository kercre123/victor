#include <sipp.h>
#include <sippMacros.h>
#include <filters/fP422Fp16U16Conv/fP422Fp16U16Conv.h>

//##########################################################################################
void svufP422Fp16U16Conv (SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 i;
    FP422Fp16U16ConvParam *param = (FP422Fp16U16ConvParam*) fptr->params;
    //Input RGBW lines (parent[0] must be the 3xplane RGBW)
    half  *inW   = (half *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo); 

    //Output lines RGBW
    UInt16   *outW = (UInt16 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);

    //the actual processing
    for(i=0; i<fptr->sliceWidth; i++)
    {  
       outW[i] =  (UInt16)((float)inW[i] * (float)param->maxVal);
    }
}
