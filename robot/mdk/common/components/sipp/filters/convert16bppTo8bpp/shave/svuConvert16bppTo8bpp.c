#include <sipp.h>
#include <sippMacros.h>
#include <filters/convert16bppTo8bpp/convert16bppTo8bpp.h>

#define CLAMPU8(x) (x < 0 ? 0 : x > 255 ? 255 : x)

//##########################################################################################
void svuConvert16bppTo8bpp (SippFilter *fptr, int svuNo, int runNo)
{
  //Input RGBW lines (parent[0] must be the 3xplane RGBW)
    UInt16 *inR   = (UInt16 *)WRESOLVE(fptr->dbLinesIn[0][runNo&1][0], svuNo); 
    UInt16 *inG   = inR + fptr->parents[0]->planeStride;
    UInt16 *inB   = inG + fptr->parents[0]->planeStride;
	UInt16 *inW   = inB + fptr->parents[0]->planeStride;

  //Output lines RGBW
    UInt8  *outR = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);
    UInt8  *outG = outR + fptr->planeStride;
    UInt8  *outB = outG + fptr->planeStride;
	UInt8  *outW = outB + fptr->planeStride;

	UInt32 i;
   //the actual processing
    for(i=0; i<fptr->sliceWidth; i++)
    {  
        outR[i] =  CLAMPU8(inR[i] >> 2);
        outG[i] =  CLAMPU8(inG[i] >> 2);
        outB[i] =  CLAMPU8(inB[i] >> 2);
		outW[i] =  CLAMPU8(inW[i] >> 2);
    }

}
