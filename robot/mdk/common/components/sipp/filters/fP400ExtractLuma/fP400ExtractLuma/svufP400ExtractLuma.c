#include <sipp.h>
#include <sippMacros.h>
#include <filters/fP400ExtractLuma/fP400ExtractLuma.h>

//##########################################################################################
void svufP400ExtractLuma (SippFilter *fptr, int svuNo, int runNo)
{
   UInt32 i;
   FP400ExtractLumaParam *param = (FP400ExtractLumaParam*) fptr->params;

   //Input RGBW lines (parent[0] must be the 3xplane RGBW)
   UInt16 *inW   = ((UInt16 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo)) + 
                   fptr->parents[0]->planeStride * param->channelPlace; 

   //Output lines RGBW
   half  *outW = (half *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);

   //the actual processing
   for(i=0; i<fptr->sliceWidth; i++)
   {  
      outW[i] =  clamp01ForP((half)((float)inW[i] / (float)param->maxVal));
   }
}
