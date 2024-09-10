/*
  ==============================================================================

    EnvelopeDetector.h
    Created: 13 Jan 2017 3:11:58pm
    Author:  KrotosMacMini

  ==============================================================================
*/

#ifndef ENVELOPEDETECTOR_H_INCLUDED
#define ENVELOPEDETECTOR_H_INCLUDED

#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include "RingBuffer.h"

enum DetectionMode
{
    Peak,
    Rms
};


/** An envelope detection class.
 *
 *  This requires setting attack and release times, the detection mode used, the current
 *  sampling rate, a window size and a lookahead size in case these options are used
 */
 
 class EnvelopeDetector
{
public:
    EnvelopeDetector();
	~EnvelopeDetector();
    
    void init(AK::IAkPluginMemAlloc* Allocator);
    
    void setAttackTime(float newAttackTime);
    
    void setReleaseTime(float newReleaseTime);
    
    void setSampleRate(float newSampleRate);
    
    void setDetectMode(DetectionMode newDetection);
    
    void setLookaheadStatus(bool newLookaheadStatus);
    
    void setLookahead(int newLookahead);
    
    float getReleaseGain();
    
    void getEnvelope(float* data, float* envelope, int numSamples);

private:
    const int MAX_LOOKAHEAD   { 256 };
    const int MAX_BUFFER_SIZE { 512 };
    
    float getRmsEnvelope(int index);
    float getPeakEnvelope(int index);
    float averageWithLookaheadBuffer(int index);
    
    float getHistoryRmsEnvelope(int index);

    float calculateAttackGain(float newAttackTime, float sampleRate);
    float calculateReleaseGain(float newReleaseTime, float sampleRate);
    
    float* m_inputBuffer;
    float m_attackTime;
    float m_releaseTime;
    float m_attackGain;
    float m_releaseGain;
    float m_sampleRate;
    float m_envelopeSample { 0.0f };
    
    int   m_windowSize  {512};

	RingBuffer* m_rmsWindow{ NULL };
	RingBuffer* m_rmsHistoryWindow{ NULL };

    
    int   m_lookahead   { 128 };
    
    float m_rmsLastTotal     {0.0f};
    int m_rmsIteration       {0};
    
    float m_rmsHistoryLastTotal     {0.0f};
    int m_rmsHistoryIteration       {0};
    
    
    bool  m_lookAheadIsOn   {false};
    
    DetectionMode m_detectMode;

	float* m_delayLine{ NULL };

    AK::IAkPluginMemAlloc* m_allocator{NULL};
};




#endif  // ENVELOPEDETECTOR_H_INCLUDED
