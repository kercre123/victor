#include <sipp.h>
#include <sippMacros.h>
#include <filters/positionKernel/positionKernel.h>

	UInt8 maskAddr[320];
	UInt8 pixelValue;
	UInt32 pixelPosition;
	UInt8 status;

void svuPositionKernel(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *input[1];		
    UInt32 i;
    positionKernelParam *param = (positionKernelParam*)fptr->params;
		
	//the input line
	input[0] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	
    //the output line
#ifdef SIPP_USE_MVCV
    outputMvCV[0] = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#else
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#endif
   
#if 0

    //copy-paste
    for(i = 0; i < fptr->sliceWidth; i++)
	{
        output[i] = input[0][i];
	}
	
	#else

		#ifdef SIPP_USE_MVCV 	
		 for(i = 0; i < fptr->sliceWidth; i++)
	{
        outputMvCV[0][i] = 0;
	}
			

			pixelPos_asm(input, param->maskAddr, fptr->sliceWidth, param->pixelValue, &param->pixelPosition, param->status);
			//outputMvCV[0][0] = param->pixelValue;
			outputMvCV[0][0] = param->status;
			
		#else	
		
			for(i = 0; i < fptr->sliceWidth; i++)
			{
				output[i] = input[0][i];
			}

		#endif
#endif
}