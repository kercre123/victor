#include <sipp.h>
#include <sippMacros.h>
#include <filters/conv15x1/conv15x1.h>


void svuConv15x1(SippFilter *fptr, int svuNo, int runNo)
{

#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *iline[15];
    Conv15x1Param *param = (Conv15x1Param*)fptr->params;
	
    UInt32 i;

    //the input line
	for(i = 0; i < 15; i++)
	{
		iline[i]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][i], svuNo);
	}
	
    //the output line
#ifdef SIPP_USE_MVCV
    outputMvCV[0] = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#else
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#endif
  
 
#if 0
    //copy-paste
    for(i = 0; i < fptr->sliceWidth; i++)
        output[i] = iline[0][i];
	#else

		#ifdef SIPP_USE_MVCV 	
		
			Convolution15x1_asm(iline, outputMvCV, (half*)param->cMat, fptr->sliceWidth);	
		           
		#else		
			for(i = 0; i < fptr->sliceWidth; i++)
			{
				output[i] = iline[0][i];
			}
		#endif
#endif
}