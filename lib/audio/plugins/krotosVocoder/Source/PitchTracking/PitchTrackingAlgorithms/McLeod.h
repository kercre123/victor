/*
  ==============================================================================

    McLeod.h
    Created: 4 Nov 2016 4:02:43pm
    Author:  KrotosMacMini

  ==============================================================================
*/

#ifndef MCLEOD_H_INCLUDED
#define MCLEOD_H_INCLUDED

#include "PitchTrackerAbstractClass.h"
#include <vector>
#include <float.h>
#include <cfloat>

class McLeod : public PitchTrackerAbstractClass
{
public:
    McLeod ();
    McLeod (double sampleRate, int bufferSize);
    ~McLeod();

    void prepareToPlay(float sampleRate, int blockSize, AK::IAkPluginMemAlloc* Allocator);
    float processBlockAndGetPitch(vector<float> buffer);

	float processBlockAndGetPitch(float* fftBuffer);
    
private:
    const float DEFAULT_SAMPLE_RATE {44100};
    const int   MAX_BUFFER_SIZE     {1024};
    const float SMALL_CUTOFF        {0.5};
    const double CUTOFF              {0.97};             //0.97 is default
    const float LOWER_PITCH_CUTOFF  {40};               //Lowest pitch estimation [Hz]
    const double PI                  {3.14159265358979323846};
    const double TWO_PI              {2*PI};
    
    int bufferSize;
    int peakPositionsIndex;
    int periodEstimatesIndex;
    int amplitudeEstimatesIndex;
    
    float interpolatedX, interpolatedY;
    
    double sampleRate;

    std::vector<int>   peakPositions;
    std::vector<float> mcLeodBuffer;
    std::vector<float> periodEstimates;
    std::vector<float> amplitudeEstimates;
    std::vector<float> window;
    
    void calculateNormalisedSquareDifferenceFunction (std::vector<float> buffer);
    void peakPicking();
    void parabolicInterpolation                      (int tauEstimation);
    float processBlockAndGetPitch(kiss_fft_cpx* buffer);
};

    


#endif  // MCLEOD_H_INCLUDED
