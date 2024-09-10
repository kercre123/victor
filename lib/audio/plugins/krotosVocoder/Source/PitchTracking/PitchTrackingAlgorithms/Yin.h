/*
  ==============================================================================

    Yin.h
    Created: 4 Nov 2016 4:02:29pm
    Author:  KrotosMacMini

  ==============================================================================
*/

#ifndef YIN_H_INCLUDED
#define YIN_H_INCLUDED

#include "PitchTrackerAbstractClass.h"
#include <vector>

class Yin : public PitchTrackerAbstractClass
{
    
public:
    Yin ();
    Yin (double sampleRate, int bufferSize);
    ~Yin();
    
    void prepareToPlay(float sampleRate, int blockSize, AK::IAkPluginMemAlloc* Allocator);
    float processBlockAndGetPitch(vector<float> buffer);
	
	float processBlockAndGetPitch(float* fftBuffer);
    
private:
    const double DEFAULT_SAMPLE_RATE { 44100 };
    const int   MAX_BUFFER_SIZE      { 1024  };
    
    int     bufferSize;
    float   probability;
    double  threshold;
    double  sampleRate;
    
    std::vector<float>  yinBuffer;
    
    void    calculateDifferenceFunction (vector<float> buffer);
    void    calculateCumulativeMeanNormalisedDifference ();
    float   getParabolicInterpolation (int tauEstimation);
    void    findBestLocalEstimate ();
    float   getProbability        ();
    int     getThreshold          ();
    float processBlockAndGetPitch(kiss_fft_cpx* buffer);
};


#endif  // YIN_H_INCLUDED
