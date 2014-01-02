#include <sipp.h>
#include <sippMacros.h>
#include <filters/gauss/gauss.h>

void gauss(UInt8** inLine, UInt8** out, UInt32 width)
{
	float gaus_matrix[3] = {1.4, 3.0, 3.8 };
	int i, j;
	float outLine1[1920 + 4];
	UInt8 *outLine;


	outLine=*out;

	for (i = 0; i < width + 4; i++)
	{
        outLine1[i] = (((inLine[0][i] + inLine[4][i]) * gaus_matrix[0]) + ((inLine[1][i] + inLine[3][i]) * gaus_matrix[1]) + (inLine[2][i]  * gaus_matrix[2]));
	}

    for (j = 0; j<width;j++)
    {
        outLine[j] = (UInt8)((((outLine1[j] + outLine1[j+4]) * gaus_matrix[0]) + ((outLine1[j+1] + outLine1[j+3]) * gaus_matrix[1]) + (outLine1[j+2]  * gaus_matrix[2]))/159);
    }
    return;
}




void svuGauss(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *input[5];	
	
    UInt32 i;
	
	//the 5 input lines
	input[0] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	input[1] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
	input[2] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
	input[3] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][3], svuNo);
	input[4] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][4], svuNo);
		
    //the output line

    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV 	
		
			gauss_asm(input, &output, fptr->sliceWidth);
			
#else		
			
			gauss(input, &output, fptr->sliceWidth);
		
#endif
}