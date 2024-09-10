//=====================================================================================
/**
 * Copyright Krotos LTD 2015
 *
 * All rights reserved.
 */
//=====================================================================================

#ifndef __Krotos__Utilities__
#define __Krotos__Utilities__
#ifndef ORBIS
#define M_PI 3.14159265359
#endif
//=====================================================================================
static bool isPositivePowerOfTwo(int x) __attribute__ ((unused));

static bool isPositivePowerOfTwo(int x)
{
    // if x is positive
    if (x > 0)
    {
        // while x is even and larger than 1
        while (((x % 2) == 0) && x > 1)
        {
            // divide by 2
            x = x / 2;
        }
        
        // if x is a power of 2 x will now be equal to 1
        return (x == 1);
    }
    else // otherwise, x is 0 or less, so return false
    {
        return false;
    }
}

//=====================================================================================
static float phaseWrap(float p)
{
    while (p <= (-M_PI))
    {
        p += (2*M_PI);
    }

    while (p > M_PI)
    {
        p -= (2*M_PI);
    }
    
    return p;
}

#endif
