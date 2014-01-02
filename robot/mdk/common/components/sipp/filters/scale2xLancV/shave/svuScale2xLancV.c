#include <sipp.h>
#include <sippMacros.h>

static float kern[4] = { -1.0f/16.0f,
                          9.0f/16.0f,
                          9.0f/16.0f,
                         -1.0f/16.0f };

//#############################################################################
void svuScl2xLancV(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32  x, ln, p;

  //Input RGB lines (parent[0] must be the 3xplane RGB)
    UInt8 *in[4];
    UInt8 *out;

  //Here: must WRESOLVE all 4 lines required
  //Initially iline_RGB[] point to RED; will add fptr->parents[0]->planeStride
  //          to move to next input plane
   for(ln=0; ln<4; ln++)
   {
      in[ln] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][ln], svuNo); 
   }
   //Initially points to CR, will add fptr->planeStride to go to next plane
   out = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);



  //The low level func implementation
   for(p=0; p<fptr->nPlanes; p++)
   {
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //Compute current plane
       if((runNo & 1) == 0)
       {//Odd line, just copy paste
           for(x=0; x<fptr->sliceWidth; x++)
               out[x] = in[1][x];
       }
       else{
           for(x=0; x<fptr->sliceWidth; x++)
           {
                out[x] = clampU8(kern[0] * in[0][x] +
                                   kern[1] * in[1][x] +
                                   kern[2] * in[2][x] +
                                   kern[3] * in[3][x] );
           }
       }

     //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     //Move to next plane (R to G, or G to B)
        for(ln=0; ln<4; ln++)
          in[ln] += fptr->parents[0]->planeStride;
 
       //Moving to next output chroma plane (R/G/B)
          out += fptr->planeStride;
   }
} 
