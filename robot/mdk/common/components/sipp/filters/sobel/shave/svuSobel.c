///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     main leon file
///

#include <sipp.h>
#include <sippMacros.h>
#include <filters/sobel/sobel.h>

void sobelFilterImplementation (UInt8 *inLine[3], UInt8 *outLine, UInt32 widthLine)
{
    UInt32 i,x,y;
    float sx = 0, sy = 0, s = 0;
    float sum;

    //sobel matrix
    int VertSobel[3][3] = {
            {-1, 0, 1},
            {-2, 0, 2},
            {-1, 0, 1}
    };

    int HorzSobel[3][3] = {
            {-1, -2, -1},
            { 0,  0,  0},
            { 1,  2,  1}
    };

    //Go on the whole line
    for (i = 1; i < (widthLine - 1); i++)
    {
        sx =    VertSobel[0][0] * inLine[0][i - 1] + VertSobel[0][1] * inLine[0][i] + VertSobel[0][2] * inLine[0][i + 1] +
                VertSobel[1][0] * inLine[1][i - 1] + VertSobel[1][1] * inLine[1][i] + VertSobel[1][2] * inLine[1][i + 1] +
                VertSobel[2][0] * inLine[2][i - 1] + VertSobel[2][1] * inLine[2][i] + VertSobel[2][2] * inLine[2][i + 1];

        sy =    HorzSobel[0][0] * inLine[0][i - 1] + HorzSobel[0][1] * inLine[0][i] + HorzSobel[0][2] * inLine[0][i + 1] +
                HorzSobel[1][0] * inLine[1][i - 1] + HorzSobel[1][1] * inLine[1][i] + HorzSobel[1][2] * inLine[1][i + 1] +
                HorzSobel[2][0] * inLine[2][i - 1] + HorzSobel[2][1] * inLine[2][i] + HorzSobel[2][2] * inLine[2][i + 1];
        s = sqrtf(sx * sx + sy * sy);
        outLine[i] = clampU8(s);
    }

}

//##########################################################################################
void svuSobel(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *outputMvCV[1];
    UInt8 *output;
    UInt8 *iline[3];

    //the 3 input lines
    iline[0] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2] = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
    //the output line
#ifdef SIPP_USE_MVCV
    outputMvCV[0] = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#else
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);
#endif

#if 0
    //copy-paste
    for (i = 0; i < fptr->sliceWidth; i++)
        output[i] = iline[1][i];
#else

#ifdef SIPP_USE_MVCV
    sobel_asm(iline, outputMvCV, fptr->sliceWidth);
#else
    sobelFilterImplementation(iline, output, fptr->sliceWidth);
#endif
#endif

}
