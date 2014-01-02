#include <sipp.h>
#include <sippMacros.h>
#include <filters/lut12to16/lut12to16.h>

void LUT12to16(UInt16** src, UInt16** dest, const UInt16* lut, UInt32 width, UInt32 height)
{
    int i, j;
    UInt16* in_line = *src;
    UInt16* out_line = *dest;

    for (j = 0; j < width; j++)
    {
        out_line[j] = lut[in_line[j]&0x0FFF];
    }

}

void svuLut12to16(SippFilter *fptr, int svuNo, int runNo)
{
    UInt16 *output;
    UInt16 *iline[1];
    Lut12to16Param *param = (Lut12to16Param*)fptr->params;
  

//the 3 input lines
    iline[0]=(UInt16 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    
//the output line
    output = (UInt16 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);


#ifdef SIPP_USE_MVCV

    LUT12to16_asm(iline, &output, param->lutValue, fptr->sliceWidth, 1); 
	
#else
    
	LUT12to16(iline, &output, param->lutValue, fptr->sliceWidth, 1);
	
#endif

}


