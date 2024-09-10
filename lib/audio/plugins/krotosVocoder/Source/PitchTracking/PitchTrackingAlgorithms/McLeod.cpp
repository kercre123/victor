/*
  ==============================================================================

    McLeod.cpp
    Created: 4 Nov 2016 4:02:48pm
    Author:  KrotosMacMini

  ==============================================================================
*/

#include "McLeod.h"
#include "float.h"

using namespace std;

McLeod::McLeod()
{
    prepareToPlay(DEFAULT_SAMPLE_RATE, MAX_BUFFER_SIZE, NULL);
}

McLeod::McLeod(double sampleRate, int bufferSize)
{
    prepareToPlay(sampleRate,bufferSize, NULL);
}

McLeod::~McLeod()
{
}

void McLeod::prepareToPlay(float newSampleRate, int newBufferSize, AK::IAkPluginMemAlloc* Allocator)
{
    sampleRate = newSampleRate;
    bufferSize = newBufferSize;
    
    mcLeodBuffer.resize(bufferSize);
    peakPositions.resize(bufferSize);
    periodEstimates.resize(bufferSize);
    amplitudeEstimates.resize(bufferSize);
    window.resize(bufferSize);
    
    for (int i=0; i<bufferSize; ++i)
    {
        mcLeodBuffer[i]       = 0.0f;
        peakPositions[i]      = 0;
        periodEstimates[i]    = 0.0f;
        amplitudeEstimates[i] = 0.0f;

        window[i] = 0.5f * (1.0f - cos(TWO_PI * i / (bufferSize) ) );

    }
}

float McLeod::processBlockAndGetPitch(vector<float> InputBuffer)
{
    int tauEstimation =     -1;
    float pitchEstimation = -1.0f;

    float maxAmplitude = - FLT_MAX;
    
    for (int i=0; i<InputBuffer.size(); ++i)
    {
        mcLeodBuffer[i] = InputBuffer[i] * window[i]; // added the window option!
    }
    
    peakPositionsIndex      = 0;
    periodEstimatesIndex    = 0;
    amplitudeEstimatesIndex = 0;
    //
    calculateNormalisedSquareDifferenceFunction(mcLeodBuffer);
    //
    peakPicking();
    //
    for (int i=0; i<peakPositionsIndex; ++i)
    {
        tauEstimation = peakPositions[i];
        maxAmplitude = fmax(maxAmplitude,mcLeodBuffer[i]);
        
        if (mcLeodBuffer[i] > SMALL_CUTOFF)
        {
            parabolicInterpolation(tauEstimation);
            amplitudeEstimates[amplitudeEstimatesIndex++] = interpolatedY;
            periodEstimates[periodEstimatesIndex++]       = interpolatedX;
            maxAmplitude = fmax(maxAmplitude,interpolatedY);
        }
    }
    //
    if (periodEstimatesIndex != 0)
    {
        float threshold = CUTOFF * maxAmplitude;
        
        int periodIndex = 0;
        for (int i=0; i<amplitudeEstimatesIndex; ++i)
        {
            if (amplitudeEstimates[i] > threshold)
            {
                periodIndex = i;
                break;
            }
        }
        pitchEstimation = sampleRate / periodEstimates[periodIndex];
        if (pitchEstimation < LOWER_PITCH_CUTOFF) pitchEstimation = -1;
    }
    
    return pitchEstimation;
}

void McLeod::calculateNormalisedSquareDifferenceFunction (std::vector<float> buffer)
{
    for (int tau=0; tau< bufferSize; ++tau)
    {
        float autocorrelationSum  = 0;
        float squareDifferenceSum = 0;
        
        for (int index = 0; index < bufferSize-tau; ++index)
        {
            autocorrelationSum  +=  buffer[index] * buffer[index+tau];
            squareDifferenceSum +=  buffer[index] * buffer[index]  + buffer[index+tau]*buffer[index+tau];
            
        }
        
        mcLeodBuffer[tau] = 2 * autocorrelationSum / squareDifferenceSum;
    }
}

void McLeod::peakPicking()
{
    int position = 0;
    int currentMaxPosition = 0;
    
    while (position < (bufferSize-1)/3  && mcLeodBuffer[position] > 0.0f)  // Initial area until we find the first zero crossing (positive -> negative)
    {
        position++;
    }
    
    while (position < (bufferSize - 1) && mcLeodBuffer[position] <= 0.0f) // Find second zero-crossing (negative -> positive)
    {
        position++;
    }
    
    if (position == 0) position = 1;
    
    while (position < bufferSize - 1)   // We have found the first positively sloped zero-crossing so now we'll look for local maxima
    {
        if (mcLeodBuffer[position] > mcLeodBuffer[position-1] && mcLeodBuffer[position] >= mcLeodBuffer[position+1]) // if a local maximum, ...
        {
            if      (currentMaxPosition == 0)                                   currentMaxPosition = position;
            else if (mcLeodBuffer[position] > mcLeodBuffer[currentMaxPosition]) currentMaxPosition = position;
        }
        
        position++;
        
        // Find the next zero-crossing
        if (position < bufferSize - 1 && mcLeodBuffer[position] <= 0.0f)
        {
            if (currentMaxPosition > 0 ) // If we found the next zero-crossing (now negatively sloped) and we have found a maximum, we register it as a candidate and reset the current position
            {
                peakPositions[peakPositionsIndex++] = currentMaxPosition;
                currentMaxPosition = 0;
            }
            
            while (position < bufferSize-1 && mcLeodBuffer[position] <= 0.0f) // "Skip" to the next z.c. (positively sloped) to start looking for candidate peaks again
            {
                position++;
            }
        }
        
    }
    
    if (currentMaxPosition > 0) peakPositions[peakPositionsIndex++] = currentMaxPosition; // Register the very last candidate peak
}

void McLeod::parabolicInterpolation(int tauEstimation)
{
    // In the McLeod method we need to implement the parabolic estimation differently since we need to keep track of
    // the abscissas and ordinates at the interpolation points so that we register them as period and max amplitude candidates

    float previous  = mcLeodBuffer[tauEstimation-1];
    float current   = mcLeodBuffer[tauEstimation];
    float next      = mcLeodBuffer[tauEstimation+1];
    
    float divisor = next + previous - 2*current;
    
    if (divisor == 0.0f)
    {
        interpolatedX = tauEstimation;
        interpolatedY = mcLeodBuffer[tauEstimation];
    }
    else
    {
        interpolatedX =  tauEstimation  + (previous - next) / (2 * divisor);
        interpolatedY =  mcLeodBuffer[tauEstimation] - ((previous-next)*(previous-next)) / (8*divisor);
    }
}

float McLeod::processBlockAndGetPitch(kiss_fft_cpx* buffer)
{
    return -1.0f;
}

float McLeod::processBlockAndGetPitch(float* fftBuffer)
{
	return -1.0f;
}


