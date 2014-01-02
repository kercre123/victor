///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Core kernels
///
/// This is the API to the MvCV kernels library
///



#include <mv_types.h>
#include <core.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __cplusplus
namespace mvcv
{
extern "C"
#endif
void minMaxKernel(u8** in, u32 width, u32 height, u8* minVal, u8* maxVal, u8* maskAddr)
{
    u8* in_1;
    in_1 = in[0];
    u8 minV = 0xFF;
    u8 maxV = 0x00;
    u32 i, j;

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

void pixelPos(u8** srcAddr, u8* maskAddr, u32 width, u8 pixelValue, u32* pixelPosition, u8* status)
{
    int j;
    u8* src;
    u8* mask;
    src = srcAddr[0];
    u32 location = 0;
    *status = 0;

    for(j = width-1; j >= 0; j--)
    {
        if (maskAddr[j] != 0)
        {
            if(src[j] == pixelValue)
            {
                location = j;
                *status = 0x11;
            }
        }
    }
    *pixelPosition = location ;
    return;
}

void minMaxPos(u8** in, u32 width, u8* minVal, u8* maxVal, u32* minPos, u32* maxPos, u8* maskAddr)
{
    u8* in_1;
    in_1 = in[0];
    u8 minV = 0xFF;
    u8 maxV = 0x00;
    u32 minLoc;
    u32 maxLoc;
    u32 i, j;

    for(j = 0; j < width; j++)
    {
        if((maskAddr[j]) != 0)
        {
            if (in_1[j] < minV)
            {
                minV = in_1[j];
                minLoc = j;
            }

            if (in_1[j] > maxV)
            {
                maxV = in_1[j];
                maxLoc = j;
            }
        }
    }
        *minVal = minV;
        *minPos = minLoc;
        *maxVal = maxV;
        *maxPos = maxLoc;
    return;
}

void meanstddev(u8** in, float *mean, float *stddev, u32 width)
{
    float sum;
    float square_sum;
    float f0, f1;
    u8    *line = in[0];
    int   i;

    sum = square_sum = 0.0f;

    for(i = 0; i < width; i++)
    {
        sum        += (float)line[i];
        square_sum += (((float)line[i]) * ((float)line[i]));
    }

    *mean = sum / (float)width;
    f0 = square_sum / (float)width;
    f1 = (*mean) * (*mean);
    if(__builtin_isgreater(f0, f1) == 1)
    {
        *stddev = sqrt(f0 - f1);
    }
    else
    {
        *stddev = 0;
    }
	return;
}

#ifdef __cplusplus
}
#endif
