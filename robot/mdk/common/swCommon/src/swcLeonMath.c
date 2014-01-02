///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API for some required Leon Math functions
///
///
///
///
///

#include "swcLeonMath.h"

//Taylor series: sin(a) = a - a^3/(3!)
//#######################################################################
float swcMathSinf(float angle)
{
  float   R;
  R = angle * 0.0174532925199432957f; // * (PI/180)
  return (R - R*R*R*0.16666666666666666f); // div (6!)
}

//#######################################################################
//Taylor series: cos(a) = 1 - a^2/(2!) +  a^4/(4!)
//#######################################################################
float swcMathCosf(float angle)
{
  float   R;
  R = angle * 0.0174532925199432957f; // * (PI/180)
  return (1.0f - R*R*0.5f + R*R*R*R*0.0416666666666666f);
}

u32 swcIPow(u32 base, u32 exp)
{
    u32 result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

double swcLongLongToDouble(unsigned long long  longVal)
{
#if 0 
    // This function can be removed if glibc is routinely included in MDK
    return (double)longVal;
#else
    double result;
    result = (unsigned long )(longVal >> 32);
    result *= 0x100000000ull;
    result += (unsigned long)(longVal);
    return result;
#endif
}

