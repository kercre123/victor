#include <sipp.h>
#include <sippMacros.h>
#include <filters/fP401DnsGetRefLut/fP401DnsGetRefLut.h>

#define MAX_LUT_IDX ((1<<10) - 1)

//##########################################################################################
void svufP401DnsGetRefLut(SippFilter *fptr, int svuNo, int runNo)
{
   UInt32 x;

   FP401DnsGetRefLutParam *param = (FP401DnsGetRefLutParam *)fptr->params;

   //Input Y line
   half *inY  = (half*)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);

   //Output Y ref
   UInt8 *outY = (UInt8*)WRESOLVE(fptr->dbLineOut[runNo&1],       svuNo);

   for(x=0; x<fptr->sliceWidth; x++)
   {
      int lutIdx = (int)(inY[x] * param->maxVal);
      
      outY[x] = param->lut[lutIdx];
   }
} 
