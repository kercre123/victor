#include <sipp.h>
#include <sippMacros.h>

static float kern[7] = { -1.0f/32.0f,
                          0.0f/32.0f,
                          9.0f/32.0f,
                         16.0f/32.0f,
                          9.0f/32.0f,
                          0.0f/32.0f,
                         -1.0f/32.0f };

//==========================================================================================
float vStep(UInt8 *inRGB[7], int pos)
{ //div by in_Y[x][pos] later...
    float res = kern[0]*inRGB[0][pos] +
                kern[1]*inRGB[1][pos] +
                kern[2]*inRGB[2][pos] +
                kern[3]*inRGB[3][pos] +
                kern[4]*inRGB[4][pos] +
                kern[5]*inRGB[5][pos] +
                kern[6]*inRGB[6][pos];
 
    
    return clampU8(res);
}

//==========================================================================================
void subs05sync7(SippFilter *fptr, UInt8 *in[7], UInt8 *out, int runNo)
{
   UInt32  i, ln, pl; 
   int       col, refPos;
   float     vstep[7]; //temp array for Vertical Step

   for(pl=0; pl<fptr->nPlanes; pl++)
   {
     //Process current plane
      for(i=0; i<fptr->sliceWidth; i++)
      {
        //Chromas: 1/2 downsampling, so:
         refPos = 2*i;//horizontal position

        //Vertical step:
         for(col = -3; col <= +3; col++)
           vstep[col+3] = vStep(in, refPos+col);

        //Horiz step:
         out[i] = clampU8(kern[0] * vstep[0] +
                          kern[1] * vstep[1] +
                          kern[2] * vstep[2] +
                          kern[3] * vstep[3] +
                          kern[4] * vstep[4] +
                          kern[5] * vstep[5] +
                          kern[6] * vstep[6]);
      }

    //Moving to next R/G/B input plane (luma doesn't change)
     for(ln=0; ln<7; ln++)
       in[ln] += fptr->parents[0]->planeStride;

    //Moving to next output chroma plane (R/G/B)
     out += fptr->planeStride;
   }
}
 
//##########################################################################################
void svuScl05Lanc7(SippFilter *fptr, int svuNo, int runNo)
{
    int  ln;

  //Input RGB lines (parent[0] must be the 3xplane RGB)
    UInt8 *in[7];
    UInt8 *out;

  //Here: must WRESOLVE all 7 lines required
  //Initially iline_RGB[] point to RED; will add fptr->parents[0]->planeStride
  //          to move to next input plane
   for(ln=0; ln<7; ln++)
   {
      in[ln] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][ln], svuNo); 
   }
   //Initially points to CR, will add fptr->planeStride to go to next plane
   out = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);

  //The low level func implementation
   subs05sync7(fptr, in, out, runNo);
} 
