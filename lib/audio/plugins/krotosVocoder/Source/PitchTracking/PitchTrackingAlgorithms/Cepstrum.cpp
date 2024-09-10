/*
==============================================================================

Cepstrum.cpp
Created: 8 Nov 2016 1:40:20pm
Author:  KrotosMacMini

==============================================================================
*/
#include "Cepstrum.h"

using namespace std;

Cepstrum::Cepstrum(AK::IAkPluginMemAlloc* Allocator)
{
    prepareToPlay(DEFAULT_SAMPLE_RATE, MAX_BUFFER_SIZE, Allocator);
}

Cepstrum::Cepstrum(float newHopSize,float newBufferSize,float newSampleRate, AK::IAkPluginMemAlloc* Allocator)
{
    prepareToPlay(newSampleRate, newBufferSize, Allocator);
}

Cepstrum::~Cepstrum()
{
	if (preAnalyser)
	{
        AK_PLUGIN_DELETE(m_allocator, preAnalyser);
		preAnalyser = NULL;
	}

	if (postAnalyser)
	{
        AK_PLUGIN_DELETE(m_allocator, postAnalyser);
		postAnalyser = NULL;
	}
}

void Cepstrum::prepareToPlay(float sampleRate, int blockSize, AK::IAkPluginMemAlloc* Allocator)
{
    //hopSize    = newHopeSize;
    hopSize = blockSize;
    bufferSize = blockSize;
    m_sampleRate = sampleRate;
    
    preAnalyser = AK_PLUGIN_NEW(Allocator,AudioAnalysisFramework(hopSize, bufferSize, sampleRate));
    postAnalyser = AK_PLUGIN_NEW(Allocator, AudioAnalysisFramework(hopSize, bufferSize, sampleRate));

    magnitudeSpectrum.resize(bufferSize);
    magnitudeCepstrum.resize(bufferSize);
    window.resize(bufferSize);
    cepstrumBuffer.resize(bufferSize);
    filteredCepstrum.resize(bufferSize);
    
    for (int i=0; i<bufferSize; ++i)
    {
        cepstrumBuffer[i]   = 0.0f;
        filteredCepstrum[i] = 0.0f;
        magnitudeSpectrum[i]= 0.0f;
        magnitudeCepstrum[i]= 0.0f;
        window[i] = 0.5f * (1.0f - std::cos(TWO_PI * i / (bufferSize) ) );
    }
    m_allocator = Allocator;
}
//================
float Cepstrum::processBlockAndGetPitch(vector<float> buffer)
{
    // window the input
    for (int i=0; i < buffer.size(); ++i)
    {
        cepstrumBuffer[i] = buffer[i] * window[i];
    }
    
    // Calculate the cepstrum magnitude
    calculateMagnitudeCepstrum(cepstrumBuffer);
    
    // Smoothing filter
    smoothingFilter(cepstrumBuffer,0.9);
 
    // Start looking for maxima
    float maxval =  0;
    int   maxbin = -1;
    
    float f_max = 5000;
    int indexMin = floor(m_sampleRate / f_max) + 1;
    
    for (int i = indexMin; i < bufferSize; ++i)
    {
        if (filteredCepstrum[i] > maxval)
        {
            maxval = filteredCepstrum[i];
            maxbin = i;
        }
    }
    
    if (maxbin < 0)
    {
        return -1;
    }
    
    float pitchEstimation = -1;
    
    // Parabolic interpolation of the peak index (if found)
    pitchEstimation = parabolicInterpolation(maxbin);
    
    return (m_sampleRate / (pitchEstimation + 4)); //offset based on f_max ?  (~SR/f_max)
}

void Cepstrum::calculateMagnitudeCepstrum(std::vector<float> buffer)
{

    preAnalyser->processFrame(buffer);
    magnitudeSpectrum = preAnalyser->getMagnitudeSpectrum();
    
    
    for (int i=0; i < magnitudeSpectrum.size(); ++i)
    {
        cepstrumBuffer[i] = fabs(log10f(magnitudeSpectrum[i]*magnitudeSpectrum[i] + epsilon));
    }
    
    postAnalyser->processFrame(cepstrumBuffer);
    magnitudeCepstrum = postAnalyser->getMagnitudeSpectrum();
    //magnitudeCepstrum = postAnalyser->getRealFFT();
    
    for (int i=0; i < magnitudeSpectrum.size(); ++i)
    {
        cepstrumBuffer[i] = magnitudeCepstrum[i]*magnitudeCepstrum[i];
    }
    
}

void Cepstrum::smoothingFilter(std::vector<float> input, float coefficient)
{
    filteredCepstrum[0] = input[0];
    for (int i=1; i<filteredCepstrum.size(); ++i)
    {
        filteredCepstrum[i] = filteredCepstrum[i-1] + coefficient * (input[i] - filteredCepstrum[i-1]);
    }
}

float Cepstrum::parabolicInterpolation(int peakEstimation)
{
    float previous  = filteredCepstrum[peakEstimation-1];
    float current   = filteredCepstrum[peakEstimation];
    float next      = filteredCepstrum[peakEstimation+1];
        
    float divisor = next + previous - 2*current;
        
    if (divisor == 0.0f)
    {
        return peakEstimation;
    }

    
    float newEstimation = peakEstimation + (previous - next) / (2 * divisor);
    return newEstimation;
}

float Cepstrum::processBlockAndGetPitch(kiss_fft_cpx* buffer)
{
    return -1.0f;
}

float Cepstrum::processBlockAndGetPitch(float* fftBuffer)
{
	return -1.0f;
}



