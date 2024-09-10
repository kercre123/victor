/*
  ==============================================================================

    EnvelopeDetector.cpp
    Created: 13 Jan 2017 3:11:58pm
    Author:  KrotosMacMini

  ==============================================================================
*/

#include "EnvelopeDetector.h"
#include <math.h>

EnvelopeDetector::EnvelopeDetector()
{
}

EnvelopeDetector::~EnvelopeDetector()
{
	if (m_rmsWindow != NULL)
	{
        AK_PLUGIN_DELETE(m_allocator, m_rmsWindow);
        m_rmsWindow = NULL;
	}

	if (m_rmsHistoryWindow != NULL)
	{
        AK_PLUGIN_DELETE(m_allocator, m_rmsHistoryWindow);
        m_rmsHistoryWindow = NULL;
	}

	if (m_delayLine != NULL)
	{
        AK_PLUGIN_FREE(m_allocator, m_delayLine);
        m_delayLine = NULL;
	}

}

void EnvelopeDetector::init(AK::IAkPluginMemAlloc* Allocator)
{
    m_rmsWindow = AK_PLUGIN_NEW(Allocator, RingBuffer(MAX_BUFFER_SIZE, Allocator));
    m_rmsWindow->clear(0.0f);

    m_rmsHistoryWindow = AK_PLUGIN_NEW(Allocator, RingBuffer(MAX_LOOKAHEAD, Allocator));
    m_rmsHistoryWindow->clear(0.0f);
    
    m_inputBuffer = NULL;
    
    m_delayLine = (float*)AK_PLUGIN_ALLOC(Allocator, sizeof(float)*MAX_LOOKAHEAD);
    for (int i = 0; i<MAX_LOOKAHEAD; ++i)
    {
        m_delayLine[i] = 0.0f;
    }
    
    m_allocator = Allocator;
}

void EnvelopeDetector::setAttackTime(float newAttackTime)
{
    m_attackTime = newAttackTime;
    m_attackGain = calculateAttackGain(m_attackTime, m_sampleRate);
};

void EnvelopeDetector::setReleaseTime(float newReleaseTime)
{
    m_releaseTime = newReleaseTime;
    m_releaseGain = calculateAttackGain(m_releaseTime, m_sampleRate);
};

void EnvelopeDetector::setSampleRate(float newSampleRate)
{
    m_sampleRate  = newSampleRate;
    m_attackGain  = calculateAttackGain(m_attackTime, m_sampleRate);
    m_releaseGain = calculateAttackGain(m_releaseTime, m_sampleRate);
};

void EnvelopeDetector::setLookaheadStatus(bool newLookaheadStatus)
{
    m_lookAheadIsOn = newLookaheadStatus;
}

void EnvelopeDetector::setLookahead(int newLookahead)
{
    m_lookahead = newLookahead;
}

void EnvelopeDetector::setDetectMode(DetectionMode newDetection)
{
    m_detectMode = newDetection;
}

float EnvelopeDetector::getRmsEnvelope(int index)
{
    if(m_inputBuffer == NULL)
    {
        return 0.0f;
    }
    float sampleSquared = m_inputBuffer[index] * m_inputBuffer[index];
    float total = 0.0f;
    float rmsValue = 0.0f;
    
    if (m_rmsIteration < m_rmsWindow->getLength()-1)
    {
        total = m_rmsLastTotal + sampleSquared;
        rmsValue = sqrtf((1.0f / (index +1)) * total);
    }
    else
    {
        total = m_rmsLastTotal + sampleSquared - m_rmsWindow->read();
        total = fabs(total);
        rmsValue = sqrtf((1.0f / m_rmsWindow->getLength()) * total);
    }
    
    m_rmsWindow->write(sampleSquared);
    m_rmsLastTotal = total;
    m_rmsIteration++;
    
    return rmsValue;
}


float EnvelopeDetector::getPeakEnvelope(int index)
{
    if(m_inputBuffer == NULL)
    {
        return 0.0f;
    }
    return fabs(m_inputBuffer[index]);
}

float EnvelopeDetector::averageWithLookaheadBuffer(int index)
{
    if(m_inputBuffer == NULL)
    {
        return 0.0f;
    }
        
    m_delayLine[index % m_lookahead] = m_inputBuffer[index];
    float history = getHistoryRmsEnvelope(index);
    
    return history;
}

float EnvelopeDetector::getHistoryRmsEnvelope(int index)
{
    float sampleSquared = m_delayLine[index] * m_delayLine[index];
    float total = 0.0f;
    float rmsValue = 0.0f;
    
    if (m_rmsHistoryIteration < m_rmsHistoryWindow->getLength() - 1)
    {
        total = m_rmsHistoryLastTotal + sampleSquared;

        rmsValue = sqrtf((1.0f / (index +1)) * total);
    }
    else
    {
        total = m_rmsHistoryLastTotal + sampleSquared - m_rmsHistoryWindow->read();
        total = fabs(total);
        rmsValue = sqrtf((1.0f / m_rmsWindow->getLength()) * total);
    }
    
    m_rmsHistoryWindow->write(sampleSquared);
    m_rmsHistoryLastTotal = total;
    m_rmsHistoryIteration++;
    
    return rmsValue;
}

void EnvelopeDetector::getEnvelope(float* data, float* envelope, int numSamples)
{
    m_inputBuffer = data;
    
    for (int i = 0; i < numSamples; i++)
    {
        float envelopeIn;
        
        if (m_detectMode == Rms)
        {
            envelopeIn = getRmsEnvelope(i);
        }        
        else
        {
            envelopeIn = getPeakEnvelope(i);
        }
        
        if (true == m_lookAheadIsOn)
        {
            envelopeIn = 0.5f * (envelopeIn + averageWithLookaheadBuffer(i));
        }
        
        if (m_envelopeSample < envelopeIn)
        {
            m_envelopeSample *= m_attackGain;
            m_envelopeSample += (1.0f - m_attackGain) * envelopeIn;
        }
        else
        {
            m_envelopeSample *= m_releaseGain;
            m_envelopeSample += (1.0f - m_releaseGain) * envelopeIn;
        }
        
        envelope[i] = m_envelopeSample;
    }
}

float EnvelopeDetector::calculateAttackGain(float newAttackTime, float sampleRate)
{
    float attackGain = expf(logf(0.01f) / (newAttackTime * sampleRate));
    return attackGain;
}

float EnvelopeDetector::calculateReleaseGain(float newReleaseTime, float sampleRate)
{

    float releaseGain = expf(logf(0.01f)/ (newReleaseTime * sampleRate));
    return releaseGain;
}
