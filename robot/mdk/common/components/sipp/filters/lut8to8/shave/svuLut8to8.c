#include <sipp.h>
#include <sippMacros.h>
#include <filters/lut8to8/lut8to8.h>

void LUT8to8(UInt8** src, UInt8** dest, const UInt8* lut, UInt32 width, UInt32 height)
{
    int i, j;
    UInt8* in_line  = *src;
    UInt8* out_line = *dest;
    for (j = 0; j < width; j++)
    {
    	out_line[j] = lut[in_line[j]];
    }
}

void svuLut8to8(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *iline[1];
    Lut8to8Param *param = (Lut8to8Param*)fptr->params;

    //the input line
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    
  //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
    LUT8to8_asm(iline, &output, param->lutValue, fptr->sliceWidth, 1); 
#else
    LUT8to8(iline, &output, param->lutValue, fptr->sliceWidth, 1);
#endif
}



