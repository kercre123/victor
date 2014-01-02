#include <sipp.h>
#include <sippMacros.h>
#include <filters/accumulateWeighted/accumulateWeighted.h>


void svuAccumulateWeighted(SippFilter *fptr, int svuNo, int runNo)
{

#ifdef SIPP_USE_MVCV
	UInt8 *input1MvCV[1], *input2MvCV[1];
    float *outputMvCV[1];
#else
    float *output;
#endif
	UInt32 i;
    UInt8 *iline[2];
	float *inLFloat = (float *)WRESOLVE((void*)fptr->dbLinesIn[2][runNo&1][0], svuNo);
	
	AccumulateWeightedParam *param = (AccumulateWeightedParam*)fptr->params;
	
//the 2 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[1][runNo&1][0], svuNo);
	
// The output line
#ifdef SIPP_USE_MVCV
      outputMvCV[0] = (float *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
      input1MvCV[0] =  iline[0];
      input2MvCV[0] =  iline[1];
	  
	  for(int i = 0; i < 320 * 240; i++)
	  {
		outputMvCV[0][i] = 0.0f;
	  }
#else
    output = (float*)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#endif

 #if 0
    //copy-paste
    for(i=0; i<fptr->sliceWidth; i++)
        output[i] = iline[1][i];
  #else

#ifdef SIPP_USE_MVCV
	/*for(i=0; i<fptr->sliceWidth; i++)
	{
		outputMvCV[0][i] = inLFloat[i];
	}*/
    AccumulateWeighted_asm(input1MvCV, input2MvCV, (float**)outputMvCV, fptr->sliceWidth, 0.4); 
#else
    //copy-paste
        for(i=0; i<fptr->sliceWidth; i++)
            output[i] = iline[1][i];
#endif

  #endif
}



