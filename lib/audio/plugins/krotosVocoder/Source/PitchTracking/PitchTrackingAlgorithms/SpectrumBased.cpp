/*
==============================================================================

SpectrumBased.cpp
Created: 4 Nov 2016 4:02:43pm
Author:  KrotosMacMini

==============================================================================
*/

#include "SpectrumBased.h"
#include "KrotosVectorOperations.h"

#include <algorithm>

using namespace std;

SpectrumBased::SpectrumBased()
{
}

SpectrumBased::~SpectrumBased()
{
    if(m_medianFilterBuffer != NULL)
    {
        AK_PLUGIN_DELETE(m_allocator, m_medianFilterBuffer);
        m_medianFilterBuffer = NULL;
    }
}

void SpectrumBased::prepareToPlay(float sampleRate, int blockSize, AK::IAkPluginMemAlloc* Allocator)
{
    m_medianFilterBuffer = AK_PLUGIN_NEW(Allocator, RingBuffer(m_medianFilterOrder, Allocator) );

    m_samplingRate = sampleRate;
    m_fftSize = blockSize;

	m_fft.initialise(blockSize);
   
    m_fftMagnitude.resize(m_fftSize);

    m_internalBuffer.resize(m_fftSize);

    m_niquistBinIndex = m_fftSize / 2;
    
    m_allocator = Allocator;
}

float SpectrumBased::processBlockAndGetPitch(vector<float> buffer)
{
    FloatVectorOperations::copy(reinterpret_cast<float*>(m_internalBuffer.data()), buffer.data(), m_fftSize);

	m_fft.calculateFFT(reinterpret_cast<float*>(m_internalBuffer.data()));
    float pitch = processBlockAndGetPitch(m_internalBuffer.data());

    return pitch;
}

float SpectrumBased::processBlockAndGetPitch(kiss_fft_cpx* buffer)
{
    for (int i = 0; i <= m_niquistBinIndex; i++)
    {
        m_fftMagnitude[i] = buffer[i].r*buffer[i].r + buffer[i].i*buffer[i].i; //Squared magnitude
    }

    int minBin = m_minFrequency * static_cast<float>(m_fftSize) / m_samplingRate;
    int maxBin = m_maxFrequency * static_cast<float>(m_fftSize) / m_samplingRate;

    auto indexMaxIndex = std::distance(m_fftMagnitude.data(), std::max_element(m_fftMagnitude.data() + minBin, m_fftMagnitude.data() + maxBin));

    float fractionalBinIndex = findMaxLocationUsingquadraticInterpolation(m_fftMagnitude[indexMaxIndex - 1], m_fftMagnitude[indexMaxIndex], m_fftMagnitude[indexMaxIndex + 1]);

    float interpolatedBinIndex = static_cast<float>(indexMaxIndex) + fractionalBinIndex;

    float currentFrequency = interpolatedBinIndex * m_samplingRate / static_cast<float>(m_fftSize);

    m_medianFilterBuffer->write(currentFrequency);

    float frequency = getMedianFilterValue();

    return frequency;
}

float SpectrumBased::processBlockAndGetPitch(float* fftBuffer)
{
	for (int i = 0; i <= m_niquistBinIndex; i++)
	{
		m_fftMagnitude[i] = fftBuffer[(i*2)]* fftBuffer[(i*2)] + fftBuffer[(i*2)+1]* fftBuffer[(i*2)+1]; //Squared magnitude
	}

	int minBin = m_minFrequency * static_cast<float>(m_fftSize) / m_samplingRate;
	int maxBin = m_maxFrequency * static_cast<float>(m_fftSize) / m_samplingRate;

	auto indexMaxIndex = std::distance(m_fftMagnitude.data(), std::max_element(m_fftMagnitude.data() + minBin, m_fftMagnitude.data() + maxBin));


	float fractionalBinIndex = findMaxLocationUsingquadraticInterpolation(m_fftMagnitude[indexMaxIndex - 1], m_fftMagnitude[indexMaxIndex], m_fftMagnitude[indexMaxIndex + 1]);

	float interpolatedBinIndex = static_cast<float>(indexMaxIndex) + fractionalBinIndex;

	float currentFrequency = interpolatedBinIndex * m_samplingRate / static_cast<float>(m_fftSize);

	m_medianFilterBuffer->write(currentFrequency);

	float frequency = getMedianFilterValue();

	return frequency;
}

bool SpectrumBased::IsPowerOfTwo(int x)
{
    return (x & (x - 1)) == 0;
}

//Source: https://www.dsprelated.com/freebooks/sasp/Quadratic_Interpolation_Spectral_Peaks.html
float SpectrumBased::findMaxLocationUsingquadraticInterpolation(float yMinusOne, float yZero, float yPlusOne)
{
    float a = yMinusOne;
    float b = yZero;
    float g = yPlusOne;

    float denominator = a - 2.0f*b + g;

    float peakLocationInBins;

    if (denominator == 0.0f)
    {
        peakLocationInBins = 0.0f;
    }
    else
    {
        peakLocationInBins = 0.5f * (a - g) / denominator;
    }

    return peakLocationInBins;
}

float SpectrumBased::getMedianFilterValue()
{
    std::vector<float> pitchValues(m_medianFilterBuffer->getBufferPointer(), m_medianFilterBuffer->getBufferPointer() + m_medianFilterBuffer->getLength());

    std::sort(pitchValues.begin(), pitchValues.end());

    int medianIndex = floor(pitchValues.size() / 2.0f);
    float medianValue = pitchValues[medianIndex];

    return medianValue;
}
