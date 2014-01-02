#include <sipp.h>
#include <sippMacros.h>
#include <filters/erodeGeneric/erodeGeneric.h>


void svuErodeGeneric(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *source[15];	
    ErodeGenericParam *param = (ErodeGenericParam*)fptr->params;
	
    UInt32 i;
	
	//the FILTER_SIZE input lines
	for(i = 0; i < param->filterSize; i++)
	{
	source[i] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][i], svuNo);
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
	{
        output[i] = source[param->filterSize / 2][i];
	}
	#else

		#ifdef SIPP_USE_MVCV 	
		
			Erode_asm(source, outputMvCV, (UInt8**)(param->eMat), fptr->sliceWidth, (UInt32)param->filterSize, param->filterSize);
		
		#else		
			for(i = 0; i < fptr->sliceWidth; i++)
			{
				output[i] = source[param->filterSize / 2][i];
			}
		#endif
#endif
}