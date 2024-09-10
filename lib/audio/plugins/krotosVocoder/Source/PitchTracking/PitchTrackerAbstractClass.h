#ifndef PITCHTRACKERABSTRACTCLASS_H_INCLUDED
#define PITCHTRACKERABSTRACTCLASS_H_INCLUDED

/*
==============================================================================

PitchTrackerABstractClass.h
Created: 4 Nov 2016 4:02:29pm
Author:  KrotosMacMini

==============================================================================
*/

class PitchTrackerAbstractClass
{
public:
    PitchTrackerAbstractClass() {};
    virtual ~PitchTrackerAbstractClass() {};

    virtual void prepareToPlay(float sampleRate, int blockSize) = 0;
    virtual float processBlockAndGetPitch(AudioSampleBuffer buffer) = 0;
    virtual float processBlockAndGetPitch(FFT::Complex* buffer) = 0;
	virtual float processBlockAndGetPitch(std::vector<float>* real, std::vector<float> *imag) = 0;
};

#endif
