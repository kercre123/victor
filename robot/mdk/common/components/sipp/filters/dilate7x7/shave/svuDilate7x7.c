#include <sipp.h>
#include <sippMacros.h>
#include <filters/dilate7x7/dilate7x7.h>


void svuDilate7x7(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *iline[7];
    Dilate7x7Param *param = (Dilate7x7Param*)fptr->params;
    UInt32 i;

    //the 7 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
    iline[3]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][3], svuNo);
    iline[4]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][4], svuNo);
    iline[5]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][5], svuNo);
    iline[6]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][6], svuNo);

	
	
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
	UInt32 k = 7;
	printf("%d width=*******\n",fptr->sliceWidth);
    //Dilate7x7_asm(iline, outputMvCV, (UInt8**)(param->dMat), fptr->sliceWidth, k); 
	Dilate7x7_asm(iline, outputMvCV, (UInt8**)(param->dMat), 10, k); 
	/* for(i=0; i<fptr->sliceWidth; i++)
		outputMvCV[i] = 0; */
	printf("%d width=*******\n",fptr->sliceWidth);
#else
    //copy-paste
        for(i=0; i<fptr->sliceWidth; i++)
            output[i] = iline[1][i];
#endif

  #endif
}



