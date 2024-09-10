//=====================================================================================
/**
 * Copyright Krotos LTD 2015
 *
 * All rights reserved.
 */
//=====================================================================================

#ifndef __Krotos__LowpassFilter__
#define __Krotos__LowpassFilter__

#include <math.h>
#include <assert.h>

//=====================================================================================
class LowpassFilter
{
public:
    
    /** Constructor */
    LowpassFilter();
    
    /** Configures the filter given the normalised cutoff frequency
     * @param normalisedCutoff should be in terms of the sampling freqeuency and so should be between 0 and 0.5
     */
    void configure(float normalisedCutoff);
    
    /** Filter a single sample
     * @param x the sample to filter
     * @returns the filtered sample
     */
    float processSample(float x);
    
    
private:
    
    float a1,a2;
    float b0,b1,b2;
    
    float x_1;
    float x_2;
    
    float y_1;
    float y_2;
};

#endif /* defined(__Krotos_Audio_Analysis_Test__LowpassFilter__) */
