#include <sipp.h>
#include <sippMacros.h>
#include <filters/lut12to8/lut12to8.h>


void LUT12to8(UInt16** src, UInt8** dest, const UInt8* lut, UInt32 width, UInt32 height)
{
    int i, j;
    UInt16* in_line = *src;
    UInt8* out_line = *dest;

    for (j = 0; j < width; j++)
    {
        out_line[j] = lut[in_line[j]&0x0FFF];
    }

}
void svuLut12to8(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt16 *iline[1];
    Lut12to8Param *param = (Lut12to8Param*)fptr->params;
    UInt32 i;

    //the input line
    iline[0]=(UInt16 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    

  //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
    LUT12to8_asm(iline, &output, param->lutValue, fptr->sliceWidth, 1); 
#else
    LUT12to8(iline, &output, param->lutValue, fptr->sliceWidth, 1);

  #endif
}


