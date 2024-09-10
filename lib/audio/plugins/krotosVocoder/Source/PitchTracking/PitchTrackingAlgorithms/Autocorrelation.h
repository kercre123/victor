/*
  ==============================================================================

    SpectralAutocorrelation.h
    Created: 9 Nov 2016 2:48:05pm
    Author:  KrotosMacMini

  ==============================================================================
*/

#ifndef AUTOCORRELATION_H_INCLUDED
#define AUTOCORRELATION_H_INCLUDED


#include "../audio-analysis-framework/FastFourierTransform.h"
#include "PitchTrackerAbstractClass.h"

class Autocorrelation : public PitchTrackerAbstractClass
{
public:
    Autocorrelation();
    Autocorrelation(double newSampleRate, int newBufferSize);
    ~Autocorrelation();
    
    void prepareToPlay(float sampleRate, int blockSize, AK::IAkPluginMemAlloc* Allocator);
    float processBlockAndGetPitch(vector<float> buffer);
	float processBlockAndGetPitch(float* fftBuffer);
    
private:
    const int     MAX_BUFFER_SIZE      {1024 };
    const double  PI                   {3.14159265358979323846};
    const double  TWO_PI               {2*PI};
    const double  DEFAULT_SAMPLE_RATE  {44100};
    
    float processBlockAndGetPitch(kiss_fft_cpx* buffer);

    void getAutocorrelation (std::vector<float> data);
    void smoothingFilter    (std::vector<float> input,  float coefficient);
    
    float getParabolicInterpolation(int estimation);
    
    std::vector<float> audioFrame;
    std::vector<float> autocorrelationBuffer;
    std::vector<float> smoothedAutocorrelation;
    std::vector<float> window;
    
    float pitchEstimate {-1.0f};
    float tauEstimate   {-1.0f};
    
    int position        {-1};
        
    int bufferSize;
    double sampleRate;
};

#endif  // AUTOCORRELATION_H_INCLUDED
