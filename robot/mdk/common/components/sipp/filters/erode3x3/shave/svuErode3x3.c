#include <sipp.h>
#include <sippMacros.h>
#include <filters/erode3x3/erode3x3.h>


void svuErode3x3(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *source[3];	
    Erode3x3Param *param = (Erode3x3Param*)fptr->params;
	
    UInt32 i;
	
	//the 3 input lines
	source[0] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	source[1] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
	source[2] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
		
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
        output[i] = source[1][i];
	}
	#else

		#ifdef SIPP_USE_MVCV 	
		
			Erode3x3_asm(source, outputMvCV, (UInt8**)(param->eMat), fptr->sliceWidth, 3);
		
		#else		
			for(i = 0; i < fptr->sliceWidth; i++)
			{
				output[i] = source[1][i];
			}
		#endif
#endif
}