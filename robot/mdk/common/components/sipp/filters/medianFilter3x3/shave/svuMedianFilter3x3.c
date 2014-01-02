#include <sipp.h>
#include <sippMacros.h>
#include <filters/medianFilter3x3/medianFilter3x3.h>

/// median blur filter - Filter, histogram based method for median calculation
/// @param[in] widthLine  - width of input line
/// @param[out]outLine    - array of pointers for output lines
/// @param[in] inLine     - array of pointers to input lines
void medianFilter3x3(UInt32 widthLine, UInt8 **outLine, UInt8 ** inLine)
{
	UInt32 i = 0;
	UInt8 *out;
    int j = 0;
    int histogram[256];
    int e = 0;
    UInt32 MED_HEIGHT = 3;
    UInt32 MED_WIDTH  = 3;
    UInt32 MED_LIMIT = 5;

    out = *outLine;

	for (i=0; i<256; i++) histogram[i] = 0;
	// build initial histogram
	for (i=0; i<MED_HEIGHT; i++)
	{
		for (j=0; j<MED_WIDTH; j++)
		{
			e =inLine[i][j-(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	for (i=0; i<widthLine; i++)
	{
		e = 0;
		j = 0;
		// use histogram
		while (j<256)
		{
			e+=histogram[j];
			if (e<MED_LIMIT)
			{
				j++;
			} else
			{
				out[i] = j;
				j = 256;
			}
		}
		// substract previous values from histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i-(MED_WIDTH>>1)];
			histogram[e]--;
		}
		// add next values to histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i+1+(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	return;
}

void svuMedianFilter3x3(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *iline[3];
    UInt32 i;

    //the 3 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
    

  //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
	medianFilter3x3_asm(fptr->sliceWidth, &output, iline);//aplying the kernel
#else
    medianFilter3x3(fptr->sliceWidth, &output, iline);
#endif
}



