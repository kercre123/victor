#include <sipp.h>
#include <sippMacros.h>
#include <filters/integralImageSqSumF32/integralImageSqSumF32.h>


void svuIntegralImageSqSumF32(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *iline[1];
    IntegralImageSqSumF32Param *param = (IntegralImageSqSumF32Param*)fptr->params;
    UInt32 i;

    //the 1 input line
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    

  //the output line
#ifdef SIPP_USE_MVCV
    outputMvCV[0] = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#else
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#endif

  #if 0
    //copy-paste
    for(i=0; i<fptr->sliceWidth; i++)
        output[i] = iline[1][i];
  #else

#ifdef SIPP_USE_MVCV
    integralimage_sqsum_f32_asm(iline, outputMvCV, param->sumVect, fptr->sliceWidth); 
#else
    //copy-paste
        for(i=0; i<fptr->sliceWidth; i++)
            output[i] = iline[1][i];
#endif

  #endif
}



