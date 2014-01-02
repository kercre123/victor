#include <sipp.h>
#include <sippMacros.h>
#include <filters/erode7x7/erode7x7.h>


void svuErode7x7(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *source[7];	
    Erode7x7Param *param = (Erode7x7Param*)fptr->params;
	
    UInt32 i;
	
	//the 7 input lines
	source[0] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	source[1] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
	source[2] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
	source[3] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][3], svuNo);
	source[4] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][4], svuNo);
	source[5] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][5], svuNo);
	source[6] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][6], svuNo);
		
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
        output[i] = source[3][i];
	}
	
	#else

		#ifdef SIPP_USE_MVCV 	
		
			Erode7x7_asm(source, outputMvCV, (UInt8**)(param->eMat), fptr->sliceWidth, 7);
			
		#else		
			for(i = 0; i < fptr->sliceWidth; i++)
			{
				output[i] = source[3][i];
			}

		#endif
#endif
}