#include <sipp.h>
#include <sippMacros.h>
#include <filters/accumulateSquare/accumulateSquare.h>

void AccumulateSquare(UInt8** srcAddr, UInt8** maskAddr, float** destAddr, UInt32 width, UInt32 height)
{
	UInt32 i,j;
	UInt8* src;
	UInt8* mask;
	float* dest;

	src = *srcAddr;
	mask = *maskAddr;
	dest = *destAddr;

	for (i = 0; i < height; i++){
		for (j = 0;j < width; j++){
			if(mask[j + i * width]){
				dest[j + i * width] =  dest[j + i * width] + src[j + i * width] * src[j + i * width];
			}
		}
	}
	return;
} 

void svuAccumulateSquare(SippFilter *fptr, int svuNo, int runNo)
{
	UInt32 i;
    UInt8 *iline1[1];
    UInt8 *iline2[1];
    float *output;

	//the 2 input lines
    iline1[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline2[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[1][runNo&1][0], svuNo);
	
    output = (float*)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV

    AccumulateSquare_asm(iline1, iline2, &output, fptr->sliceWidth, 1); 
#else
    AccumulateSquare(iline1, iline2, &output, fptr->sliceWidth, 1);
#endif

}



