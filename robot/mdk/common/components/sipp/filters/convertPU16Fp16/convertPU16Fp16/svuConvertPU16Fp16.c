#include "core/sipp.h"
#include "core/sippMacros.h"
#include "filters/convertPU16Fp16/convertPU16Fp16.h"

#define MAX_U16_VAL ((1<<10)) // this depend by nr of pixels 10 for now - have to be a parameter at some point

//##########################################################################################
void svuConvertPU16Fp16 (SippFilter *fptr, int svuNo, int runNo)
{
   UInt32 i;
  //Input RGBW lines (parent[0] must be the 3xplane RGBW)
    UInt16 *inW   = (UInt16 *)WRESOLVE(fptr->dbLinesIn[0][runNo&1][0], svuNo); 

  //Output lines RGBW
    half  *outW = (half *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);

   //the actual processing
    for(i=0; i<fptr->sliceWidth; i++)
    {  
		outW[i] =  (half)inW[i] / (half)MAX_U16_VAL;
    }
}
