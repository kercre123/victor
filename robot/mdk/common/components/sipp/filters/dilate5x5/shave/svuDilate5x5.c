#include <sipp.h>
#include <sippMacros.h>
#include <filters/dilate5x5/dilate5x5.h>


void svuDilate5x5(SippFilter *fptr, int svuNo, int runNo)
{
#ifdef SIPP_USE_MVCV
    UInt8 *outputMvCV[1];
#else
    UInt8 *output;
#endif
    UInt8 *iline[5];
    Dilate5x5Param *param = (Dilate5x5Param*)fptr->params;
    UInt32 i;

    //the 5 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
    iline[3]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][3], svuNo);
    iline[4]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][4], svuNo);

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
    Dilate5x5_asm(iline, outputMvCV, (UInt8**)(param->dMat), fptr->sliceWidth, 5); 
#else
    //copy-paste
        for(i=0; i<fptr->sliceWidth; i++)
            output[i] = iline[1][i];
#endif

  #endif
}



