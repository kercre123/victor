#include <sipp.h>
#include <sippMacros.h>
#include <filters/minMaxValue/minMaxValue.h>

void minMaxKernel(UInt8** in, UInt32 width, UInt32 height, UInt8* minVal, UInt8* maxVal, UInt8* maskAddr)
{
    UInt8* in_1;
    in_1 = in[0];
    UInt8 minV = 0xFF;
    UInt8 maxV = 0x00;
    UInt32 i, j;

    for(j = 0; j < width; j++)
    {
        if((maskAddr[j]) != 0)
        {
            if (in_1[j] < minV)
            {
                minV = in_1[j];
            }

            if (in_1[j] > maxV)
            {
                maxV = in_1[j];
            }
        }
    }

    if (minVal != NULL)
    {
        *minVal = minV;
    }
    if (maxVal != NULL)
    {
        *maxVal = maxV;
    }
    return;
}

void svuMinMaxValue(SippFilter *fptr, int svuNo, int runNo)
{

    UInt8 *output;
    UInt8 *input[1];		
    UInt32 i;
	
    minMaxValParam *param = (minMaxValParam*)fptr->params;
		
	//the input line
	input[0] =(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
	
    
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
    for(i=0; i < fptr->sliceWidth; i++)
	   output[i] = 0x00;
	
    

#ifdef SIPP_USE_MVCV 	
		
		minMaxKernel_asm(input, (fptr->sliceWidth), 1, &param->minVal, &param->maxVal, param->maskAddr);
		output[0] = param->minVal;
		output[1] = param->maxVal;
#else
		
		minMaxKernel(input, (fptr->sliceWidth), 1, &param->minVal, &param->maxVal, param->maskAddr);
		output[0] = param->minVal;
		output[1] = param->maxVal;
#endif
}