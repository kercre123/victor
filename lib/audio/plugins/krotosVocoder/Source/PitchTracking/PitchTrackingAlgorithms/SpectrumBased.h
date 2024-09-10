#ifndef SPECTRUMBASED_H_INCLUDED
#define SPECTRUMBASED_H_INCLUDED

/*
==============================================================================

SpectrumBased.h
Created: 4 Nov 2016 4:02:43pm
Author:  KrotosMacMini

==============================================================================
*/

#include <memory>
#include <vector>

#include "PitchTrackerAbstractClass.h"
#include "RingBuffer.h"
#include "audio-analysis-framework/FastFourierTransform.h"

class SpectrumBased : public PitchTrackerAbstractClass
{
public:
    SpectrumBased();
    ~SpectrumBased();

    void prepareToPlay(float sampleRate, int blockSize, AK::IAkPluginMemAlloc* Allocator);
    float processBlockAndGetPitch(vector<float> buffer);
    float processBlockAndGetPitch(kiss_fft_cpx* buffer);
	float processBlockAndGetPitch(float* fftBuffer);
private:
    //Methods
    bool IsPowerOfTwo(int x);
    float findMaxLocationUsingquadraticInterpolation(float yMinusOne, float yZero, float yPlusOne);
    float getMedianFilterValue();

    //Constants
    const float m_minFrequency { 80.0f };
    const float m_maxFrequency { 500.0f };
    const int m_medianFilterOrder{ 11 };

    //Parameters
    float m_samplingRate;
    int m_fftSize;
    int m_niquistBinIndex;

	FastFourierTransform m_fft;

    std::vector<float> m_fftMagnitude;
    std::vector<kiss_fft_cpx> m_internalBuffer;
    RingBuffer* m_medianFilterBuffer;

    AK::IAkPluginMemAlloc* m_allocator{NULL};
    

};




#endif 
