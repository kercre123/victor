#include <sipp.h>
#include <sippMacros.h>

//#############################################################################
void svuScl05BilinHV(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32  p, x;

  //Input RGB lines (parent[0] must be the 3xplane RGB)
    UInt8 *in[2];
    UInt8 *out;

  //The two input lines needed (bilinear)
    in[0] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo); 
    in[1] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo); //don't actually need this...
  
  //Initially points to CR, will add fptr->planeStride to go to next plane
    out   = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);


  //The low level implementation
   for(p=0; p<fptr->nPlanes; p++)
   {
      //================================
      //1) compute current plane
        for(x=0; x<fptr->sliceWidth; x++)
        {
           out[x+0] = in[0][x*2];
        }

      //================================
      //2) Move to next plane    
        in[0] += fptr->parents[0]->planeStride;
        in[1] += fptr->parents[0]->planeStride;
        out   += fptr->planeStride;
   }
} 
