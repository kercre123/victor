#include <sipp.h>
#include <sippMacros.h>
#include <filters/minMaxPos/minMaxPos.h>


void svuMinMaxPos(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *input[1];	
    MinMaxPosParam *param = (MinMaxPosParam*)fptr->params;
	
    UInt32 i;

	//the input line
	input[0] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	
		
    //the output line
#ifdef SIPP_USE_MVCV
    outputMvCV[0] = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
	for(i = 0; i < fptr->parents[0]->sliceWidth; i++)
				outputMvCV[0][i] = 0;
#else
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#endif
  
 
#if 0
    //copy-paste	
    for(i = 0; i < fptr->parents[0]->sliceWidth; i++)
	{
        output[i] = input[0][i];
	}
	#else

		#ifdef SIPP_USE_MVCV 	
	
			minMaxPos_asm(input, fptr->parents[0]->sliceWidth, outputMvCV[0], (UInt8*)(&outputMvCV[0][1]), (&outputMvCV[0][2]), (&outputMvCV[0][6]), (UInt8*)param->Mask);
			
		#else		
		
			for(i = 0; i < fptr->sliceWidth; i++)
			{
				output[i] = input[0][i];
			}
		#endif
#endif
}