#include <sipp.h>
#include <sippMacros.h>
#include <filters/dilateGeneric/dilateGeneric.h>


void svuDilateGeneric(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *iline[15];
    DilateGenericParam *param = (DilateGenericParam*)fptr->params;
    UInt32 i;

    //the input lines for all cases
    for (i = 0; i < param->kernelSize; i++) {
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
    for(i=0; i<fptr->sliceWidth; i++)
        output[i] = iline[1][i];
  #else

#ifdef SIPP_USE_MVCV
	
    Dilate_asm(iline, outputMvCV, (UInt8**)param->dMat, fptr->sliceWidth, param->kernelSize, 15); 
		
		#else
    //copy-paste
        for(i=0; i<fptr->sliceWidth; i++)
            output[i] = iline[1][i];
#endif

  #endif
}



