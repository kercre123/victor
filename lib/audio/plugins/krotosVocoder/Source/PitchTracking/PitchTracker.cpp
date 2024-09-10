/*
 ==============================================================================

 PitchTracker.cpp
 Created: 4 Nov 2016 4:02:29pm
 Author:  KrotosMacMini

 ==============================================================================
*/
#include "PitchTracker.h"


#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>

using namespace std;

PitchTracker::PitchTracker()
{
}

PitchTracker::~PitchTracker()
{
	if (m_detector)
	{
		AK_PLUGIN_DELETE(m_pAllocator, m_detector);
		m_detector = NULL;
	}
}

void PitchTracker::prepareToPlay(float sampleRate, int blockSize)
{
	m_sampleRate = sampleRate;
	m_blockSize = blockSize;

	if (m_pAllocator == NULL)
	{
		return;
	}

	if (m_detector != NULL)
	{
		AK_PLUGIN_DELETE(m_pAllocator, m_detector);
		m_detector = NULL;
	}

	switch (m_algorithm)
	{
	case Algorithm::Autocorrelation:
		m_detector = AK_PLUGIN_NEW(m_pAllocator, Autocorrelation());
		break;

	case Algorithm::Cepstrum:
		m_detector = AK_PLUGIN_NEW(m_pAllocator, Cepstrum(m_pAllocator));
		break;

	default:
	case Algorithm::McLeod:
		m_detector = AK_PLUGIN_NEW(m_pAllocator, McLeod());
		break;

	case Algorithm::SpectrumBased:
		m_detector = AK_PLUGIN_NEW(m_pAllocator, SpectrumBased());
		break;

	case Algorithm::Yin:
		m_detector = AK_PLUGIN_NEW(m_pAllocator, Yin());
		break;
	}
	m_detector->prepareToPlay(m_sampleRate, m_blockSize, m_pAllocator);
}

float PitchTracker::processBlockAndGetPitch(float* fftBuffer)
{
	float pitch = m_detector->processBlockAndGetPitch(fftBuffer);

	return pitch;
}

float PitchTracker::processBlockAndGetPitch(kiss_fft_cpx* buffer)
{
    float pitch = m_detector->processBlockAndGetPitch(buffer);

    return pitch;
}

float PitchTracker::processBlockAndGetPitch(vector<float> buffer)
{
    /*if (buffer.getNumChannels() == 2)
    {
        buffer.addFrom(0, 0, buffer, 1, 0, 0.5f);
    }*/

    float pitch = m_detector->processBlockAndGetPitch(buffer);

    return pitch;
}
void PitchTracker::setAlgorithm(Algorithm algorithm)
{
    if (m_algorithm != algorithm)
    {
        m_algorithm = algorithm;

        prepareToPlay(m_sampleRate, m_blockSize);
    }
}

PitchTracker::Algorithm PitchTracker::getAlgorithm()
{
    return m_algorithm;
}
