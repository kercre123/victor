#include <sipp.h>
#include <sippMacros.h>
#include <filters/cvtColor/cvtColor.h>

void svuCvtColor(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCVY;
	UInt8 *outputMvCVU;
	UInt8 *outputMvCVV;
#else
    UInt8 *output;
#endif
    UInt8 *input[1];		
    UInt32 i;
	
	//the input line
	input[0] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	
    //the output line
#ifdef SIPP_USE_MVCV
    outputMvCVY = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
	outputMvCVU = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
	outputMvCVV = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#else
    outputMvCVY = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
	outputMvCVU = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
	outputMvCVV = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#endif
   
#if 0
    //copy-paste
    for(i = 0; i < fptr->sliceWidth; i++)
	{
        output[i] = input[0][i];
	}
	
	#else

		#ifdef SIPP_USE_MVCV 	

			cvtColorKernelRGBToYUV_asm(input, outputMvCVY, outputMvCVU, outputMvCVV, fptr->sliceWidth);
			
		#else	
		
			for(i = 0; i < fptr->sliceWidth; i++)
			{
				output[i] = input[0][i];
			}

		#endif
#endif
}