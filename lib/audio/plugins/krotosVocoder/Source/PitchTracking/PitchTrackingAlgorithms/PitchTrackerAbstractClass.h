#ifndef PITCHTRACKERABSTRACTCLASS_H_INCLUDED
#define PITCHTRACKERABSTRACTCLASS_H_INCLUDED

/*
==============================================================================

PitchTrackerAbstractClass.h
Created: 4 Nov 2016 4:02:43pm
Author:  KrotosMacMini

==============================================================================
*/

#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include "audio-analysis-framework/libraries/kiss_fft/kiss_fft.h"
#include <vector>

using namespace std;

class PitchTrackerAbstractClass
{
public:
    PitchTrackerAbstractClass() {};
    virtual ~PitchTrackerAbstractClass() {};

    virtual void prepareToPlay(float sampleRate, int blockSize, AK::IAkPluginMemAlloc* Allocator) = 0;
    virtual float processBlockAndGetPitch(vector<float> buffer) = 0;
    virtual float processBlockAndGetPitch(kiss_fft_cpx* buffer) = 0;
	virtual float processBlockAndGetPitch(float* fftBuffer) = 0;
};

#endif
