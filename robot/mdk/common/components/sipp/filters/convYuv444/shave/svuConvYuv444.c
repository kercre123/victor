#include <sipp.h>
#include <sippMacros.h>

//this scale factor is used to up half[0..1] to UInt8
#define SCALE 255.0f

//##########################################################################################
void svuRgbYuv444(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 i;

  //Input RGB lines 
    half *inR = (half *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    half *inG = inR + fptr->parents[0]->planeStride;
    half *inB = inG + fptr->parents[0]->planeStride;

  //Output lines
    UInt8 *outY  = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);
    UInt8 *outU  = outY + fptr->planeStride;
    UInt8 *outV  = outU + fptr->planeStride;

  //Conversion:
    for(i=0; i<fptr->sliceWidth; i++)
    {
        outY[i] = clampZ255(inR[i] * ( 0.299f*SCALE) + inG[i]* ( 0.587f*SCALE) + inB[i] * ( 0.114f *SCALE)         );
        outU[i] = clampZ255(inR[i] * (-0.169f*SCALE) + inG[i]* (-0.332f*SCALE) + inB[i] * ( 0.500f *SCALE) + 128.0f);
        outV[i] = clampZ255(inR[i] * ( 0.500f*SCALE) + inG[i]* (-0.419f*SCALE) + inB[i] * (-0.0813f*SCALE) + 128.0f);
    }
} 
