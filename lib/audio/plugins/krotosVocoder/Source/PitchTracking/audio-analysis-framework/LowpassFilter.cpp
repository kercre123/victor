//=====================================================================================
/**
 * Copyright Krotos LTD 2015
 *
 * All rights reserved.
 */
//=====================================================================================

#include "LowpassFilter.h"

//=====================================================================================
LowpassFilter::LowpassFilter() : x_1(0), x_2(0), y_1(0), y_2(0)
{
    configure(0.3);
}

//=====================================================================================
void LowpassFilter::configure(float normalisedCutoff)
{
    assert(normalisedCutoff <= 0.5);
    assert(normalisedCutoff >= 0.0);
#ifndef ORBIS
    const float M_PI = 3.14159265359f;
#endif
    float K = tan(M_PI*normalisedCutoff);
    
    float denominator = 1 + sqrt(2)*K + (K*K);
    
    b0 = (K*K) / denominator;
    b1 = (2.*K*K) / denominator;
    b2 = b0;
    a1 = (2.*(K*K - 1.)) / denominator;
    a2 = (1 - sqrt(2)*K + (K*K)) / denominator;
}

//=====================================================================================
float LowpassFilter::processSample(float x)
{
    float y = b0*x + b1*x_1 + b2*x_2 - a1*y_1 - a2*y_2;
    
    x_2 = x_1;
    x_1 = x;
    y_2 = y_1;
    y_1 = y;
    
    return y;
}
