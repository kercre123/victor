#include <stdlib.h>
#include <sipp.h>
#include <sippMacros.h>
#include <filters/absdiff/absdiff.h>

void AbsoluteDiff(UInt8** in1, UInt8** in2, UInt8** out, UInt32 width)
{
    UInt8* in_1;
    UInt8* in_2;
    in_1= *in1;
    in_2= *in2;
	//check if the two input images have the same size

    UInt32 j;
    if (sizeof(in1) != sizeof(in2))
    {
        exit(1);
    }
    else
    {

    	for (j = 0; j < width; j++)
    	{
    		if (in_1[j] > in_2[j])
    		{
    			out[0][j] = in_1[j] - in_2[j];
    		}
    		else
    		{
    			out[0][j] = in_2[j] - in_1[j];
    		}
    	}

    }
    return;
}


void svuAbsdiff(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *iline[2];

//the 2 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[1][runNo&1][0], svuNo);

//the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
  AbsoluteDiff_asm(&iline[0], &iline[1], &output, fptr->sliceWidth);
#else
  AbsoluteDiff(&iline[0], &iline[1], &output, fptr->sliceWidth);
#endif

}
