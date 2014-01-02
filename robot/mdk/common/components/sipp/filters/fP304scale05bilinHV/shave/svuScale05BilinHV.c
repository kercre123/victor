#include <sipp.h>
#include <sippMacros.h>

//#############################################################################
void svufP304scale05bilinHV(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32  p, x;

    UInt8 *in;
    UInt8 *out;

    in = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo); 
    out   = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);

  //The low level implementation
   for(p=0; p<fptr->nPlanes; p++)
   {
      //1) compute current plane
        for(x=0; x<fptr->sliceWidth; x++)
        {
           out[x] = in[x * 2];
        }
      //2) Move to next plane    
        in  += fptr->parents[0]->planeStride;
        out += fptr->planeStride;
   }
} 
