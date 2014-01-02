#include "core/sipp.h"
#include "core/sippMacros.h"
#include "filters/convertPFp16U16/convertPFp16U16.h"

#define MAX_U16_VAL ((1<<10)) // this depend by nr of pixels 10 for now - have to be a parameter at some point

//##########################################################################################
void svuConvertPFp16U16 (SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 i;
    //Input RGBW lines (parent[0] must be the 3xplane RGBW)
    half  *inW   = (half *)WRESOLVE(fptr->dbLinesIn[0][runNo&1][0], svuNo); 

    //Output lines RGBW
    UInt16   *outW = (UInt16 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);

    //the actual processing
    for(i=0; i<fptr->sliceWidth; i++)
    {  
		outW[i] =  (UInt16)(inW[i] * (half)MAX_U16_VAL);
    }
}
