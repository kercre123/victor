/*
  ==============================================================================

    Cepstrum.h
    Created: 8 Nov 2016 1:40:20pm
    Author:  KrotosMacMini

  ==============================================================================
*/

#ifndef CEPSTRUM_H_INCLUDED
#define CEPSTRUM_H_INCLUDED
#include "../audio-analysis-framework/AudioAnalysisFramework.h"
#include "PitchTrackerAbstractClass.h"

class Cepstrum : public PitchTrackerAbstractClass
{
public:
    Cepstrum(AK::IAkPluginMemAlloc* Allocator);
    Cepstrum(float newHopSize,float newBufferSize,float newSampleRate, AK::IAkPluginMemAlloc* Allocator);
    ~Cepstrum();

    void prepareToPlay(float sampleRate, int blockSize, AK::IAkPluginMemAlloc* Allocator);
    float processBlockAndGetPitch(vector<float> buffer);

	float processBlockAndGetPitch(float* fftBuffer);
private:
    float processBlockAndGetPitch(kiss_fft_cpx* buffer);

    const int DEFAULT_HOP_SIZE       {512};
    const int MAX_BUFFER_SIZE        {1024};
    const double DEFAULT_SAMPLE_RATE {44100};
    const double  PI                 {3.14159265358979323846};
    const double  TWO_PI             {2*PI};

    double m_sampleRate;
    double epsilon = 1e-10;
    
    std::vector<float> magnitudeSpectrum;
    std::vector<float> magnitudeCepstrum;
    std::vector<float> window;
    
    int hopSize{ DEFAULT_HOP_SIZE };
    int bufferSize;
    
    std::vector<float> cepstrumBuffer;
    std::vector<float> filteredCepstrum;
    
    void calculateMagnitudeCepstrum (std::vector<float> buffer);
    void smoothingFilter            (std::vector<float> input,  float coefficient);
    float parabolicInterpolation    (int peakEstimation);

	AudioAnalysisFramework* preAnalyser{ NULL };
	AudioAnalysisFramework* postAnalyser{ NULL };
    
    AK::IAkPluginMemAlloc* m_allocator{NULL};
};



#endif  // CEPSTRUM_H_INCLUDED
