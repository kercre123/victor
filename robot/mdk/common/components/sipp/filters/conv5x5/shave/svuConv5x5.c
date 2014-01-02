#include <sipp.h>
#include <sippMacros.h>
#include <filters/conv5x5/conv5x5.h>

/// Convolution 5x5 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth      - width of input line
void Convolution5x5(UInt8** in, UInt8** out, float conv[25], UInt32 inWidth)
{
    int x, y;
    unsigned int i;
    UInt8* lines[5];
    float sum;

    //Initialize lines pointers
    lines[0] = in[0];
    lines[1] = in[1];
    lines[2] = in[2];
    lines[3] = in[3];
    lines[4] = in[4];

    //Go on the whole line
    for (i = 0; i < inWidth; i++)
    {
        sum = 0.0;
        for (x = 0; x < 5; x++)
        {
            for (y = 0; y < 5; y++)
            {
                sum += (float)(lines[x][y] * conv[x * 5 + y]);
            }
            lines[x]++;
        }
        if (sum >= 255)
            sum = 255.0;
        if (sum <= 0)
            sum = 0.0;
        out[0][i] = (UInt8)(sum);
    }
    return;
}


//##########################################################################################
void svuConv5x5(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *iline[5];
    Conv5x5Param *param = (Conv5x5Param*)fptr->params;
    UInt32 i;

    //the 3 input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    iline[1]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    iline[2]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);
	iline[3]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][3], svuNo);
    iline[4]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][4], svuNo);
  


   output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
   Convolution5x5_asm(iline, &output, param->cMat, fptr->sliceWidth);
#else
   Convolution5x5(iline, &output, param->cMat, fptr->sliceWidth);
#endif
}



