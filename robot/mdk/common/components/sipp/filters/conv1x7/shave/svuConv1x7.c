#include <sipp.h>
#include <sippMacros.h>
#include <filters/conv1x7/conv1x7.h>

/// Convolution 1x7 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth    - width of input line
void Convolution1x7(UInt8 **in, UInt8 **out, float *conv, UInt32 inWidth)
{
    UInt32   i, y;
    float sum = 0.0;
    UInt8    *input  = *in;
    UInt8    *output = *out;

    for (i = 0; i < inWidth; i++)
    {
        sum = 0.0;

        for (y = 0; y < 7; y++)
        {
            sum += (float)input[y] * conv[y];
        }
        input++;

        if (sum >= 255.0f)
            sum = 255.0f;
        if (sum <= 0.0f)
            sum = 0.0f;
        output[i] = (UInt8)(sum);
    }
    return;
}

void svuConv1x7(SippFilter *fptr, int svuNo, int runNo)
{
    UInt8 *output;
    UInt8 *iline[1];
    Conv1x7Param *param = (Conv1x7Param*)fptr->params;
    UInt32 i;

    //the input lines
    iline[0]=(UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    
    //the output line
    output = (UInt8 *)WRESOLVE((void*)fptr->dbLineOut[runNo&1], svuNo);

#ifdef SIPP_USE_MVCV
	Convolution1x7_asm(iline, &output, param->cMat, fptr->sliceWidth);
#else
    Convolution1x7(iline, &output, param->cMat, fptr->sliceWidth);
#endif
}








