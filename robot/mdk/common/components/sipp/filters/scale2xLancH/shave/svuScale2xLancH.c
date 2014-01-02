#include <sipp.h>
#include <sippMacros.h>

static float kern[4] = { -1.0f/16.0f,
                          9.0f/16.0f,
                          9.0f/16.0f,
                         -1.0f/16.0f };

//#############################################################################
void svuScl2xLancH(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 p, i;

  //Input RGB line (parent[0] must be the 3xplane RGB)
    UInt8 *in  = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo); 
    UInt8 *out = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1],    svuNo);

  //The low level implementation
   for(p=0; p<fptr->nPlanes; p++)
   {
     //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     //Do current plane (doing 2 pixel per iter)
     for(i=0; i<fptr->sliceWidth; )
     {
        out[i] = in[i/2];
        i++; //move to ODD pix

        out[i] = clampU8(kern[0] * in[i/2-1] +
                         kern[1] * in[i/2+0] +
                         kern[2] * in[i/2+1] +
                         kern[3] * in[i/2+2]);
        i++; //move to EVEN pixel
     }

     //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     //Move to next plane
        in  += fptr->parents[0]->planeStride;
        out += fptr->planeStride;
   }

} 
