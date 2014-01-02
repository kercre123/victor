#include <sipp.h>
#include <sippMacros.h>
#include <filters/pyrDown/pyrDown.h>

void pyrdown(UInt8 **inLine,UInt8 **out,int width)
{
	unsigned int gaus_matrix[3] = {16, 64,96 };
	int i, j;
	UInt8 outLine11[1920];
	UInt8 *outLine1;
	UInt8 *outLine;

	outLine1 = outLine11;
	outLine1[0] = 0;
	outLine1[1] = 0;
	outLine1 = outLine1 + 2;

	outLine=*out;

	for (i = 0; i < width; i++)
	{
        outLine1[i] = (((inLine[0][i] + inLine[4][i]) * gaus_matrix[0]) + ((inLine[1][i] + inLine[3][i]) * gaus_matrix[1]) + (inLine[2][i]  * gaus_matrix[2]))>>8;
	}

	outLine1[width] = 0;
	outLine1[width + 1] = 0;

    for (j = 0; j<width;j+=2)
    {
        outLine[j>>1] = (((outLine1[j-2] + outLine1[j+2]) * gaus_matrix[0]) + ((outLine1[j-1] + outLine1[j+1]) * gaus_matrix[1]) + (outLine1[j]  * gaus_matrix[2]))>>8;
    }

	return;
}

void svuPyrDown(SippFilter *fptr, int svuNo, int runNo)
{

    UInt8 *output;
    UInt8 *iline[3];
	
	//the 3 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);    

  //the output line
	output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
	



#ifdef SIPP_USE_MVCV
    pyrdown_asm(iline, &output,  fptr->sliceWidth); 

#else


	pyrdown(iline, &output,  fptr->sliceWidth);


  #endif
}



