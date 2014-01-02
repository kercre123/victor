#include <sipp.h>
#include <sippMacros.h>
#include <filters/integralImageSumF32/integralImageSumF32.h>

void integralimage_sum_f32(UInt8** in, UInt8** out, float* sum, UInt32 width)
{
    int i,x,y;
    UInt8* in_line;
    float* out_line;
    UInt32 s=0;
    //initialize a line pointer for one of u32 or f32
    in_line = *in;
    out_line = (float *)(*out);

    //go on the whole line and sum the pixels
    for(i=0;i<width;i++)
    {
        s=s+in_line[i]+sum[i];
        out_line[i]=(float)s;
        sum[i]=sum[i]+in_line[i];
    }

    return;
}

void svuIntegralImageSumF32(SippFilter *fptr, int svuNo, int runNo)
{
	UInt8 *output;
    UInt8 *iline[1];
    IntegralImageSumF32Param *param = (IntegralImageSumF32Param*)fptr->params;
    

    //the 1 input line
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    

  //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
    integralimage_sum_f32_asm(iline, &output, param->sumVect, fptr->sliceWidth); 
	
  #else

    integralimage_sum_f32(iline, &output, param->sumVect, fptr->sliceWidth);
  
  #endif
}



