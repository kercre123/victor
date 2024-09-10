/*
 ==============================================================================
 
 StereoSpread.h
 Created: 13 Jun 2016 1:37:25pm
 Author:  Doug
 
 ==============================================================================
 */

#ifndef STEREOSPREAD_H_INCLUDED
#define STEREOSPREAD_H_INCLUDED

#include "KrotosDspUtilityFunctions.h"

/** A Stereo Spread processor which takes a width control as a parameter and splits the signal between channels based on freq bands. 
 *
 *  It uses a large network of all-pass filters, and so isn't the most efficient of processes. The number of all-pass filters used is directly
 *  related to the perceived quality of the processing. At the time of development 25 seemed like a good number to use, but it would be wise
 *  to experiment with this if you are using this class in a new application.
 *
 *  The code uses standard RBJ filters and so the variable names have been kept consistent with the transfer function formulae.
 *
 */

struct BiQuadFilters
{
    enum FilterType
    {
        lowpass,
        highpass,
        bandpassCSG,
        bandpassCZPG,
        notch,
        allpass,
        peaking,
        lowshelf,
        highshelf
    };
};

/** A structure to hold coefficients in the standard RBJ format.
 */
template <class FloatType> struct BiQuadCoefficients
{
    FloatType b0a0, b1a0, b2a0, a1a0, a2a0;
};

/** A single Filter that is used in the Network. Implements the standard RBJ equations.
 *
 *  The process functions haven't been documented as you'll find them in any DSP textbook :)
 */
template <class FloatType> class RbjFilter
{
public:
    RbjFilter() { reset(); };
    
    void reset()
    {
        coeff.b0a0 = coeff.b1a0 = coeff.b2a0 = coeff.a1a0 = coeff.a2a0 = 0.0;
        ou1 = ou2 = in1 = in2 = 0.0f;
    }
    
    /** processBlock - out and in can be the same location */
    void processBlock (const FloatType* in, FloatType* out, int numSamples)
    {
        FloatType r;
        
        for (int i = 0; i < numSamples; ++i)
        {
            r =
            coeff.b0a0 * in[i] + // x[n] current sample
            coeff.b1a0 * in1 + // x[n-1] sample
            coeff.b2a0 * in2 - // x[n-2] sample
            coeff.a1a0 * ou1 - // y[n-1] feedback
            coeff.a2a0 * ou2;  // y[n-2] feedback
            
            in2 = in1;
            in1 = in[i];
            ou2 = ou1;
            ou1 = r;
            out[i] = r;
        }
    }
    
    inline FloatType processSample (FloatType in)
    {
        FloatType r;
        r =
        coeff.b0a0 * in + // x[n] current sample
        coeff.b1a0 * in1 + // x[n-1] sample
        coeff.b2a0 * in2 - // x[n-2] sample
        coeff.a1a0 * ou1 - // y[n-1] feedback
        coeff.a2a0 * ou2;  // y[n-2] feedback
        
        in2 = in1;
        in1 = in;
        ou2 = ou1;
        ou1 = r;
        
        removeBadFloatValuesFromStereoPair();
        
        return r;
    }
    
    /*  We're doing a lot of nested processing with maths functions that are reasonably
     *  likely to introduce infinite or NaN values. This function just gets rid of them.
     */
    void removeBadFloatValuesFromStereoPair()
    {
        ou1 = removeBadFloatValues (ou1);
        ou2 = removeBadFloatValues (ou2);
    }
    
    /**
     Begin with these user defined parameters:
     
     Fs (the sampling frequency)
     
     f0 ("wherever it's happenin', man."  Center Frequency or
     Corner Frequency, or shelf midpoint frequency, depending
     on which filter type.  The "significant frequency".)
     
     dBgain (used only for peaking and shelving filters)
     
     Q (the EE kind of definition, except for peakingEQ in which A*Q is
     the classic EE Q.  That adjustment in definition was made so that
     a boost of N dB followed by a cut of N dB for identical Q and
     f0/Fs results in a precisely flat unity gain filter or "wire".)
     
     _or_ BW, the bandwidth in octaves (between -3 dB frequencies for BPF
     and notch or between midpoint (dBgain/2) gain frequencies for
     peaking EQ)
     
     _or_ S, a "shelf slope" parameter (for shelving EQ only).  When S = 1,
     the shelf slope is as steep as it can be and remain monotonically
     increasing or decreasing gain with frequency.  The shelf slope, in
     dB/octave, remains proportional to S for all other values for a
     fixed f0/Fs and dBgain.
     */
    
    void setFilter (int const type,
                    double const frequency,
                    double const sample_rate,
                    double const q,
                    double const db_gain,
                    bool q_is_bandwidth)
    {
        double alpha(0.0), a0(0.0), a1(0.0), a2(0.0), b0(0.0), b1(0.0), b2(0.0);
        
        // peaking, lowshelf and hishelf
        if (type >= BiQuadFilters::peaking)
        {
            double const A		=	pow (10.0, (db_gain / 40.0));
            double const omega	=	2.0 * M_PI * frequency / sample_rate;
            double const tsin	=	sin (omega);
            double const tcos	=	cos (omega);
            
            if (q_is_bandwidth)
                alpha = tsin * sinh (log (2.0) / 2.0 * q * omega / tsin);
            else
                alpha = tsin / (2.0 * q);
            
            double const beta	=	sqrt (A) / q;
            
            switch (type)
            {
                case BiQuadFilters::peaking:
                    b0 = FloatType (1.0 + alpha * A);
                    b1 = FloatType (-2.0 * tcos);
                    b2 = FloatType (1.0 - alpha * A);
                    a0 = FloatType (1.0 + alpha / A);
                    a1 = FloatType (-2.0 * tcos);
                    a2 = FloatType (1.0 - alpha / A);
                    break;
                    
                case BiQuadFilters::lowshelf:
                    b0 = FloatType (A * ( (A + 1.0) - (A - 1.0) * tcos + beta * tsin));
                    b1 = FloatType (2.0 * A * ( (A - 1.0) - (A + 1.0) * tcos));
                    b2 = FloatType (A * ( (A + 1.0) - (A - 1.0) * tcos - beta * tsin));
                    a0 = FloatType ( (A + 1.0) + (A - 1.0) * tcos + beta * tsin);
                    a1 = FloatType (-2.0 * ( (A - 1.0) + (A + 1.0) * tcos));
                    a2 = FloatType ( (A + 1.0) + (A - 1.0) * tcos - beta * tsin);
                    break;
                    
                case BiQuadFilters::highshelf:
                    b0 = FloatType (A * ( (A + 1.0) + (A - 1.0) * tcos + beta * tsin));
                    b1 = FloatType (-2.0 * A * ( (A - 1.0) + (A + 1.0) * tcos));
                    b2 = FloatType (A * ( (A + 1.0) + (A - 1.0) * tcos - beta * tsin));
                    a0 = FloatType ( (A + 1.0) - (A - 1.0) * tcos + beta * tsin);
                    a1 = FloatType (2.0 * ( (A - 1.0) - (A + 1.0) * tcos));
                    a2 = FloatType ( (A + 1.0) - (A - 1.0) * tcos - beta * tsin);
                    break;
            }
        }
        else
        {
            // other filters
            double const omega	=	2.0 * M_PI * frequency / sample_rate;
            double const tsin	=	sin (omega);
            double const tcos	=	cos (omega);
            
            if (q_is_bandwidth)
                alpha = tsin * sinh (log (2.0) / 2.0 * q * omega / tsin);
            else
                alpha = tsin / (2.0 * q);
            
            a0 = 1.0 + alpha;
            a1 = -2.0 * tcos;
            a2 = 1.0 - alpha;
            
            switch (type)
            {
                case BiQuadFilters::lowpass:
                    b0 = (1.0 - tcos) / 2.0;
                    b1 = 1.0 - tcos;
                    b2 = (1.0 - tcos) / 2.0;
                    break;
                    
                case BiQuadFilters::highpass:
                    b0 = (1.0 + tcos) / 2.0;
                    b1 = - (1.0 + tcos);
                    b2 = (1.0 + tcos) / 2.0;
                    break;
                    
                case BiQuadFilters::bandpassCSG: // Constant Skirt Gain, Peak Gain is Q.
                    b0 = tsin / 2.0;
                    b1 = 0.0;
                    b2 = -tsin / 2;
                    break;
                    
                case BiQuadFilters::bandpassCZPG: // Constant 0db peak gain
                    b0 = alpha;
                    b1 = 0.0;
                    b2 = -alpha;
                    break;
                    
                case BiQuadFilters::notch:
                    b0 = 1.0;
                    b1 = -2.0 * tcos;
                    b2 = 1.0;
                    break;
                    
                case BiQuadFilters::allpass:
                    b0 = 1.0 - alpha;
                    b1 = -2.0 * tcos;
                    b2 = 1.0 + alpha;
                    break;
            }
        }
        
        coeff.b0a0 = FloatType (b0 / a0);
        coeff.b1a0 = FloatType (b1 / a0);
        coeff.b2a0 = FloatType (b2 / a0);
        coeff.a1a0 = FloatType (a1 / a0);
        coeff.a2a0 = FloatType (a2 / a0);
    };
    
    BiQuadCoefficients<FloatType>& getCoefficients() { return coeff; }
    void setCoefficients (BiQuadCoefficients<FloatType> c) { coeff = c; }
    
private:
    BiQuadCoefficients<FloatType> coeff;
    FloatType ou1, ou2, in1, in2;
};

/** The final All Pass filter is slightly different to the rest of the filters in the network. 
 *  See the online documentation for details.
 */
class FinalAllpass
{
public:
    
    float processSample(float sample)
    {
        auto lx1 = x1;
        x1 = sample;
        
        sample = -0.5f * sample + lx1 + 0.5f * y1;
        
        y1 = sample;
        
        removeBadFloatValuesFromStereoPair();
        
        return sample;
    }
    
    void removeBadFloatValuesFromStereoPair()
    {
        y1 = removeBadFloatValues(y1);
    }
    
private:
    float y1 {0.0f}, x1 {0.0f};
};

/** The network of filters to be used in the Stereo Spread algorithm.
 */
class FilterNetwork
{
public:
    
    /** The filter network is based on the standard critical bands found in any graphic EQ. 
     *  These can be altered to affect the sonic behaviour of the tool, but you want there to be some sort of 
     *  sensible relationship between them unless you're going for something a bit sci-fi!
     */
    void configure(double sr, float q)
    {
        double frequencies[numFilters] = {50.0, 150.0, 250.0, 350.0, 450.0, 570.0, 700.0, 840.0, 1000.0, 1170.0, 1370.0, 1600.0, 1850.0, 2150.0, 2500.0, 2900.0, 3400.0, 4000.0, 4800.0, 5800.0, 7000.0, 8500.0, 10500.0, 13500.0};
        
        for (int i = 0; i < numFilters; ++i)
            filters[i].setFilter(BiQuadFilters::allpass, frequencies[i], sr, q, 0.0, false);
    }
    
    /** This calls the helper functions embedded in each Filter class to remove infinite and NaN values.
     */
    void removeBadFloatValuesFromStereoPair()
    {
        for (int i = 0; i < numFilters; ++i)
            filters[i].removeBadFloatValuesFromStereoPair();
        
        finalAllpass.removeBadFloatValuesFromStereoPair();
    }
    
    /** Send a value through the Filter network.
     */
    float process(float sample, bool use21)
    {
        for (int i = 0; i < numFilters; ++i)
            sample = filters[i].processSample(sample);
        
        if (use21)
            sample = finalAllpass.processSample(sample);
        
        return sample;
    }
    
    RbjFilter<float> filters[numFilters];
    FinalAllpass finalAllpass;
    
    const int numFilters { 24 };
};

//============================================

/** This stereo spread algorithm relies on two identical networks of All Pass filters (yeah that's a *lot* of filters) to split
 *  a mono signal into a pseudo-stereo one. Frequency bands are distributed between the channels, allowing the user to control the width.
 */
class StereoSpread
{
public:
    
    template <class FloatType> struct StereoPair
    {
        FloatType l, r;
    };
    
    
    StereoSpread() noexcept
    {
        filters1.configure(44100, 0.5);
        filters2.configure(44100, 0.5);
        
        processedSample = { 0.0f, 0.0f };
    }
    
    
    void setSampleRateAndQ (double sampleRate, float Q)
    {
        filters1.configure(sampleRate, Q);
        filters2.configure(sampleRate, Q);
    }
    
    
    /** Processes a sample in place. By nature this process takes a mono in and produces a stereo out, so this 
     *  function assumes that the mono input is on the left channel.
     *
     *  @param  l   This address of the mono input that is to be processed. The value at this location 
     *              will be replaced by the left hand side of the resulting StereoPair.
     *
     *  @param  r   The address that will be used to store the right hand result. 
     *
     *  @param amount   The width value. Expected to be in the range of 0.0f - 1.0f.
     */
    
    void processSampleInPlace(float & l, float & r, float amount)
    {
        bool use21stFilter = true;
        
        auto allpass1 = filters1.process(l, use21stFilter);
        auto allpass2 = filters2.process(allpass1, use21stFilter);
        
        r = amount * l + allpass1;// * (1.0f - (amount * 0.3f));
        l = allpass1 - allpass2 * amount;
    }
    
    /** An out of place version of the process function.
     */
    void processSample(float monoInput, float amount)
    {
        bool use21stFilter = true;
        
        auto allpass1 = filters1.process(monoInput, use21stFilter);
        auto allpass2 = filters2.process(allpass1, use21stFilter);
        
        processedSample.l = amount * monoInput + allpass1;// * (1.0f - (amount * 0.3f));
        processedSample.r = allpass1 - allpass2 * amount;
    }
    
    /** Get the last value that was processed.
     */
    StereoPair<float> getProcessedSample()
    {
        return processedSample;
    }
    
    
private:
    
    FilterNetwork filters1, filters2;
    
    StereoPair<float> processedSample;
};


#endif  // STEREOSPREAD_H_INCLUDED
