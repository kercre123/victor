#include <sipp.h>
#include <sippMacros.h>
#include <filters/contrast/contrast.h>

//##########################################################################################
void svuCrop(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 i,p;
    UInt8  *in  = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo); //Input line(s)
    UInt8  *out = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1],    svuNo); //Output line(s)
  
    for(p=0; p<fptr->nPlanes; p++)
    {
     //Do current plane
      for(i=0; i<fptr->sliceWidth * fptr->bpp; i++)
      {
         out[i] = in[i];
      }

     //Move to next plane:
      in  += fptr->parents[0]->planeStride;
      out += fptr->planeStride;
    }
} 
