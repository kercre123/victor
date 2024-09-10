/*
  ==============================================================================

    SpectralAutocorrelation.cpp
    Created: 9 Nov 2016 2:48:10pm
    Author:  KrotosMacMini

  ==============================================================================
*/
#include "Autocorrelation.h"

using namespace std;

Autocorrelation::Autocorrelation()
{
    prepareToPlay(DEFAULT_SAMPLE_RATE,MAX_BUFFER_SIZE, NULL);
}

Autocorrelation::Autocorrelation(double newSampleRate,int newBufferSize)
{
    prepareToPlay(newSampleRate,newBufferSize, NULL);
}

Autocorrelation::~Autocorrelation()
{
    
}

void Autocorrelation::prepareToPlay(float newSampleRate, int newBufferSize, AK::IAkPluginMemAlloc* Allocator)
{
    sampleRate = newSampleRate;
    bufferSize = newBufferSize;
    
    // set up the buffers
    autocorrelationBuffer.resize(bufferSize);
    smoothedAutocorrelation.resize(bufferSize);
    audioFrame.resize(newBufferSize);
    // create the hanning window
    window.resize(bufferSize);
    
    for (int i=0; i<bufferSize; ++i)
    {
        window[i] = 0.5f * (1.0f - std::cos(TWO_PI * i / (bufferSize) ) );
        autocorrelationBuffer[i] = 0.0f;
        smoothedAutocorrelation[i] = 0.0f;
    }

}

float Autocorrelation::processBlockAndGetPitch(vector<float> buffer)
{
    // window the input
    for (int i=0; i<buffer.size(); ++i)
    {
        audioFrame[i] = buffer[i] * window[i];
    }
    
 
    // Simple Autocorrelation
    getAutocorrelation(audioFrame);
    
    // Smooth it out (w/ coeff. alpha = 0.9)
    smoothingFilter(autocorrelationBuffer, 0.9f);
    
    // and find the minimum
    float tmp = smoothedAutocorrelation[0];
    position = -1;
    
    for (int i=0; i<bufferSize; ++i)
    {
        if (smoothedAutocorrelation[i] < tmp)
        {
            tmp = smoothedAutocorrelation[i];
            position = i;
        }
    }
    
    pitchEstimate = -1.0f;
    tauEstimate   = -1.0f;
    
    
    if (position < 1)
    {
        pitchEstimate = -1;
        //std::cout << "F0 = " << pitchEstimate << "Hz" <<std::endl;
        return pitchEstimate;
    }
    else if (position == bufferSize-1)
    {
        tauEstimate = 2 * position;
        pitchEstimate = sampleRate / tauEstimate;
        //std::cout << "F0 = " << pitchEstimate << "Hz" <<std::endl;
        return pitchEstimate;
    }
    else
    {
        tauEstimate = 2 * getParabolicInterpolation(position);
        pitchEstimate = sampleRate / tauEstimate;
        //std::cout << "F0 = " << pitchEstimate << "Hz" <<std::endl;
        return pitchEstimate;
    }
}

float Autocorrelation::processBlockAndGetPitch(kiss_fft_cpx* buffer)
{
    return -1.0f;
}

float Autocorrelation::processBlockAndGetPitch(float* fftBuffer)
{
	return -1.0f;
}

void Autocorrelation::getAutocorrelation(std::vector<float> data)
{
    
    for (int tau = 0 ; tau < 0.5 * bufferSize; ++tau)
    {
        float sum = 0.0f;
        for(int index = 0; index < 0.5 * bufferSize; ++index)
        {
            sum += data[index] * data[index + tau];
        }
        autocorrelationBuffer[tau]  = sum / bufferSize;
    }
}

void Autocorrelation::smoothingFilter(std::vector<float> in,  float coefficient)
{
    smoothedAutocorrelation[0] = in[0];
    for (int i=1; i<bufferSize; ++i)
    {
        smoothedAutocorrelation[i] = smoothedAutocorrelation[i-1] + coefficient * (in[i] - smoothedAutocorrelation[i-1]);
    }
}

float Autocorrelation::getParabolicInterpolation(int tauEstimation)
{
    float previous  = autocorrelationBuffer[tauEstimation-1];
    float current   = autocorrelationBuffer[tauEstimation];
    float next      = autocorrelationBuffer[tauEstimation+1];
    
    float divisor = next + previous - 2*current;
    float newEstimation;

    if (divisor == 0.0f)
    {
        newEstimation = tauEstimation;
    }
    else
    {
        newEstimation =  tauEstimation  + (previous - next) / (2 * divisor);
    }

    return newEstimation;
}
