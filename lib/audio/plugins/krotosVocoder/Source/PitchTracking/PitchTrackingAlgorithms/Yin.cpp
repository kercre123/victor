/*
==============================================================================

Yin.cpp
Created: 4 Nov 2016 4:02:29pm
Author:  KrotosMacMini

==============================================================================
*/
#include "Yin.h"

using namespace std;

Yin::Yin()
{
    threshold   = 0.1;
    prepareToPlay(DEFAULT_SAMPLE_RATE, MAX_BUFFER_SIZE, NULL);
}

Yin::Yin(double sampleRate, int bufferSize)
{
    threshold   = 0.1;
    prepareToPlay(sampleRate,bufferSize, NULL);

}

Yin::~Yin()
{
    
}

void Yin::prepareToPlay(float newSampleRate, int newBufferSize, AK::IAkPluginMemAlloc* Allocator)
{
    sampleRate = newSampleRate;
    bufferSize = newBufferSize;
    
    threshold   = 0.1;
    probability = 0.8;
    
    yinBuffer.resize(bufferSize);
    for (int i=0; i<bufferSize; ++i)
    {
        yinBuffer[i] = 0.0f;
    }
}

float Yin::getProbability()
{
    return probability;
}

float Yin::processBlockAndGetPitch(vector<float> buffer)
{
    // Based on "YIN, a fundamental frequency estimator for speech and music", by Alain de Cheveigne, Hideki Kawahara.
    // The step 1 (as described by the paper) is going to be omitted as it serves as a comparative starting point for the actual algorithm
    
    for (int i=0; i < buffer.size(); ++i)
       yinBuffer[i] = 0.0f;
    
    int tauEstimation =     -1;
    float pitchEstimation = -1.0f;
    
    // Step 2: Find the difference function
    calculateDifferenceFunction(buffer);
    
    // Step 3: Find the  Cumulative mean normalized difference function
    calculateCumulativeMeanNormalisedDifference();
    
    // Step 4: Find the absolute threshold
    tauEstimation = getThreshold();
    
    // Step 5: Perform parabolic interpolation at the estimate
    if (tauEstimation != -1)
    {
        float parabolic = getParabolicInterpolation(tauEstimation);
        if (!isnan(parabolic))
        {
            pitchEstimation = sampleRate / parabolic;
        }
    }

    return pitchEstimation;
}

void Yin::calculateDifferenceFunction(vector<float> buffer)
{
    float delta;

    for (int tau = 0 ; tau < 0.5 * buffer.size(); ++tau)
    {
        for(int index = 0; index < 0.5 * buffer.size(); ++index)
        {
            delta = buffer[index] - buffer[index + tau];
            yinBuffer[tau] += delta * delta;
        }
    }
}

void Yin::calculateCumulativeMeanNormalisedDifference ()
{
    // For zero lag, the Cumulative Mean Normalised Difference is 1
    yinBuffer[0] = 1;
    
    // and for the rest of the lag values, we have to divide by the running sum
    float sum = 0.0f;
    
    for (int tau = 1; tau < /*0.5 * */ bufferSize; ++tau)
    {
        sum += yinBuffer[tau];
        yinBuffer[tau] *= tau / sum;
    }
}

int Yin::getThreshold()
{
    int tau;
    
    // According to de Cheveigne and Kawahara the threshold determines the list of
    // candidates admitted to the set, and can be interpreted as the
    // proportion of aperiodic power tolerated in a periodic signal. Thus, we need
    // to find the minimum lag at which the CMND is less than the threshold.
    for (tau = 2; tau < bufferSize-1 ; tau++)
    {
        if (yinBuffer[tau] < threshold) {
            while (tau + 1 < bufferSize && yinBuffer[tau + 1] < yinBuffer[tau])
            {
                tau++;
            }
            // Since we want the periodicity and and not aperiodicity:
            // periodicity = 1 - aperiodicity
            probability = 1 - yinBuffer[tau];
            break;
        }
    }
    
    // If no pitch was found, lag = -1
    if (tau == bufferSize-1 || yinBuffer[tau] >= threshold)
    {
        tau = -1;
        probability = 0;
    }
    
    return tau;
}

float Yin::getParabolicInterpolation(int tauEstimation)
{
    float previous  = yinBuffer[tauEstimation-1];
    float current   = yinBuffer[tauEstimation];
    float next      = yinBuffer[tauEstimation+1];

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
/*
 float Yin::getParabolicInterpolation(int tauEstimation)
{
    float newEstimation;
    
    int x_prev, x_next;
    
    
    // Boundary conditions
    if (tauEstimation < 1)
    {
        x_prev = tauEstimation;
    }
    else
    {
        x_prev = tauEstimation - 1;
    }
    
    if (tauEstimation + 1 < bufferSize)
    {
        x_next = tauEstimation + 1;
    }
    else
    {
        x_next = tauEstimation;
    }
    
    if (x_prev == tauEstimation)
    {
        if (yinBuffer[tauEstimation] <= yinBuffer[x_next])
        {
            newEstimation = tauEstimation;
        }
        else
        {
            newEstimation = x_next;
        }
    }
    else if (x_next == tauEstimation)
    {
        if (yinBuffer[tauEstimation] <= yinBuffer[x_prev])
        {
            newEstimation = tauEstimation;
        } 
        else
        {
            newEstimation = x_prev;
        }
    }
    else // actual interpolation
    {
        float y, y_prev, y_next;
        y      = yinBuffer[tauEstimation];
        y_prev = yinBuffer[x_prev];
        y_next = yinBuffer[x_next];
        
        newEstimation = tauEstimation + 0.5 * (y_prev-y_next) * (y_next + y_prev - 2 * y);
    }
    
    return newEstimation;
}
*/

void Yin::findBestLocalEstimate()
{
    
}

float Yin::processBlockAndGetPitch(kiss_fft_cpx* buffer)
{
    return -1.0f;
}

float Yin::processBlockAndGetPitch(float* fftBuffer)
{
	return -1.0f;
}




