#include <sipp.h>
#include <sippMacros.h>
#include <filters/fP405LanczosUpscale/fP405LanczosUpscale.h>

//=============================================================================
half LANCZOS_FACTOR1 = (half)0.024457f;
half LANCZOS_FACTOR2 = (half)-0.135870f;
half LANCZOS_FACTOR3 = (half)0.611413f;

half line[5000]; // point the scratch memory at some point

//=============================================================================
void hzLanczosUpscale (half *out, half *in, int widthIn)
{
   int x;
   for (x = 0; x < (widthIn); x++)
   {
      // a b c X d e f
      half a = in[x-2];
      half b = in[x-1];
      half c = in[x];
      half d = in[x+1];
      half e = in[x+2];
      half f = in[x+3];

      half value =  (a + f) * LANCZOS_FACTOR1 
                  + (b + e) * LANCZOS_FACTOR2 
                  + (c + d) * LANCZOS_FACTOR3;

      out[x*2] = clamp01ForP(in[x]);
      out[x * 2 + 1] = clamp01ForP(value);
   }   
}

//=============================================================================
void vertLanczosUpscale (half *out, half **in, int widthIn)
{
   for (int x = -3; x < (widthIn + 3); x++)
   {
      // a b c X d e f
      half a = in[0][x];
      half b = in[1][x];
      half c = in[2][x];
      half d = in[3][x];
      half e = in[4][x];
      half f = in[5][x];

      half value = (a + f) * LANCZOS_FACTOR1 
                 + (b + e) * LANCZOS_FACTOR2 
                 + (c + d) * LANCZOS_FACTOR3;
      out[x] = value;
   }       
}
        

//#############################################################################
void svufP405LanczosUpscale(SippFilter *fptr, int svuNo, int runNo)
{
   const UInt32 KERNEL_SIZE = 6;
   UInt32  ln;

   half *in[KERNEL_SIZE];
   half *out;
   half *inBuf = &line[8];
   for(ln=0; ln<KERNEL_SIZE; ln++)
   {
      in[ln] = (half *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][ln], svuNo); 
   }
   //Initially points to CR, will add fptr->planeStride to go to next plane
   out = (half *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);

   
   if ((runNo % 2) == 0)
   {
      // even lines kernel
      hzLanczosUpscale(out, in[2], (fptr->sliceWidth>>1));
   }
   else
   {
      // odd lines kernel
      vertLanczosUpscale(inBuf, in, (fptr->sliceWidth>>1));
      hzLanczosUpscale(out, inBuf, (fptr->sliceWidth>>1));
   }
}
   
