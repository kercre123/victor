#include <sipp.h>
#include <sippMacros.h>
#include <filters/equalizeHist/equalizeHist.h>

void svuEqualizeHist(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *input[1];		
    UInt32 i;
	
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

			equalizeHist_asm(input, outputMvCV, param->cum_hist, fptr->sliceWi	dth);
			
			 for (i = 0; i < 256; i++)
				output[i] = UInt32 *param->cum_hist[i];	
		#else	
		
			for(i = 0; i < fptr->sliceWidth; i++)
			{
				output[i] = input[0][i];
			}

		#endif
#endif
}