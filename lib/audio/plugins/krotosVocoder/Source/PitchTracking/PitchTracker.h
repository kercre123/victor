#ifndef PITCHTRACKER_H_INCLUDED
#define PITCHTRACKER_H_INCLUDED

/*
==============================================================================

PitchTracker.h
Created: 4 Nov 2016 4:02:29pm
Author:  KrotosMacMini

==============================================================================
*/

#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>

#include "Autocorrelation.h"
#include "Cepstrum.h"
#include "McLeod.h"
#include "SpectrumBased.h"
#include "Yin.h"

class PitchTracker
{
public:

    enum class Algorithm
    {
        Autocorrelation = 0,
        Cepstrum,
        McLeod,
        SpectrumBased,
        Yin,
        NumberOfAlgorithms
    };

    PitchTracker();
	~PitchTracker();

    void prepareToPlay(float sampleRate, int blockSize);

    float processBlockAndGetPitch(vector<float> buffer);

    float processBlockAndGetPitch(kiss_fft_cpx* buffer);

	float processBlockAndGetPitch(float* fftBuffer);

    void setAlgorithm(Algorithm algorithm);
    Algorithm getAlgorithm();

    void setAllocator(AK::IAkPluginMemAlloc* in_pAllocator) { m_pAllocator = in_pAllocator; }

private:
    //Constants
    const float  DEFAULT_SAMPLE_RATE { 44100 };
    const int    NOMINAL_BLOCK_SIZE  { 512 };
    
    //Parameters
    float     m_sampleRate { DEFAULT_SAMPLE_RATE };
    int       m_blockSize  { NOMINAL_BLOCK_SIZE };
    Algorithm m_algorithm  { Algorithm::McLeod };

	PitchTrackerAbstractClass* m_detector{ NULL };
	AK::IAkPluginMemAlloc* m_pAllocator{ NULL };
};

#endif
