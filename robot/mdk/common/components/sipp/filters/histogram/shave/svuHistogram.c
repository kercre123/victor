#include <sipp.h>
#include <sippMacros.h>
#include <filters/histogram/histogram.h>



void svuHistogram(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *iline[1];
    HistogramParam *param = (HistogramParam*)fptr->params;
    UInt32 i;
	UInt32* out;

    //the input line
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    
  //the output line
#ifdef SIPP_USE_MVCV
    outputMvCV[0] = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#else
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#endif

  #if 0
    //copy-paste
    for(i=0; i<fptr->sliceWidth; i++)
        output[i] = iline[0][i];
	
  #else

#ifdef SIPP_USE_MVCV
	out = (UInt32 *)outputMvCV;
	
	printf("----\n");
	histogram_asm(iline, param->hist, fptr->sliceWidth);
	
	 for (i = 0; i < 256; i++)
    out[i] = (UInt32 *)param->hist[i];	
	
	#else
    //copy-paste
        for(i = 0; i < fptr->sliceWidth; i++)
            output[i] = iline[0][i];	
#endif
  #endif

  }



