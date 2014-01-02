#include <sipp.h>
#include <sippMacros.h>
#include <filters/cannyEdgeDetection/cannyEdgeDetection.h>


void svuCannyEdgeDetection(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *input[9];	
	
    UInt32 i;
	
	//the 5 input lines
	input[0] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	input[1] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
	input[2] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
	input[3] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][3], svuNo);
	input[4] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][4], svuNo);
	input[5] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][5], svuNo);
	input[6] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][6], svuNo);
	input[7] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][7], svuNo);
	input[8] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][8], svuNo);
		
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
        output[i] = input[2][i];
	}
	#else

		#ifdef SIPP_USE_MVCV 	
		
			canny_asm(input, outputMvCV, param->threshold1, param->threshold2, fptr->sliceWidth);
			
		#else		
			for(i = 0; i < fptr->sliceWidth; i++)
			{
				output[i] = input[2][i];
			}

		#endif
#endif
}