#include <sipp.h>
#include <sippMacros.h>
#include <filters/dilate3x3/dilate3x3.h>

void Dilate3x3(UInt8** src, UInt8** dst, UInt8** kernel, UInt32 width, UInt32 height)
{
	int j, i1, j1;
	UInt8 max = 0;
	UInt8* row[3];


	for(j = 0;j < 3; j++)
			row[j] = src[j];

	// Determine the anchor positions which must be replaces by a maximum value
	for ( j = 0; j < width; j++)
	{
		max = 0;
		// Determine the maximum number into a k*k square
		for(i1 = 0; i1 < 3; i1++ )
		{
			for (j1 = 0; j1 < 3; j1++)
			{
				if(kernel[i1][j1]==1)
				{
					if(max < row[i1][j1+j])
                    {
						max = row[i1][j1+j];
					}
				}
			}
		}

		dst[0][j] =  max;
	}
}


void svuDilate3x3(SippFilter *fptr, int svuNo, int runNo)
{

    UInt8 *output;
    UInt8 *iline[3];
    Dilate3x3Param *param = (Dilate3x3Param*)fptr->params;
    UInt8 *kernel[] = {(*param->dMat)[0], (*param->dMat)[1], (*param->dMat)[2]};
    UInt32 i, j;

    //the 3 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);

  //the output line

    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
    Dilate3x3_asm(iline, &output, (UInt8**)(param->dMat), fptr->sliceWidth, 3);
#else

    Dilate3x3(iline, &output, kernel, fptr->sliceWidth, 3);
	
#endif

  }



