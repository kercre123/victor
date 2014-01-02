#include <sipp.h>
#include <sippMacros.h>

//#############################################################################
void svuScl2xBilinHV(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32  p, x;

  //Input RGB lines (parent[0] must be the 3xplane RGB)
    UInt8 *in[2];
    UInt8 *out;

  //The two input lines needed (bilinear)
    in[0] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo); 
    in[1] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo); 
  
  //Initially points to CR, will add fptr->planeStride to go to next plane
    out = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);


  //The low level implementation
   for(p=0; p<fptr->nPlanes; p++)
   {
      //================================
      //1) compute current plane
        if((runNo & 1) == 0)
        {//Even line (Y phase = 0, thus only need to interpolate on Horizontal 
            for(x=0; x<fptr->sliceWidth; x+=2)
            {
                out[x+0] = ((int)in[0][x/2]);
                out[x+1] = ((int)in[0][x/2] + (int)in[0][x/2+1] + 1)>>1;
            }
        }
        else
        {//Odd line, Y phase = 0.5 always
            for(x=0; x<fptr->sliceWidth; x+=2)
            {
                out[x+0] = ((int)in[0][x/2] + (int)in[1][x/2] + 1)>>1;

                out[x+1] = ((int)in[0][x/2] + (int)in[0][x/2+1] +
                            (int)in[1][x/2] + (int)in[1][x/2+1] + 3) >> 2;
            }
        }

      //================================
      //2) Move to next plane    
        in[0] += fptr->parents[0]->planeStride;
        in[1] += fptr->parents[0]->planeStride;
        out   += fptr->planeStride;
   }
  
} 
