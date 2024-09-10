//=====================================================================================
/**
 * Copyright Krotos LTD 2015
 *
 * All rights reserved.
 */
//=====================================================================================

#include "AudioAnalysisFramework.h"
#include "Utilities.h"

//=====================================================================================
AudioAnalysisFramework::AudioAnalysisFramework(int inputFrameSize, int analysisBufferSize, int fs)
: shouldCalculateFFFT(true)
, rmsCalculated(false)
, rmsValue(0)
, energyDifferenceCalculated(false)
, previousEnergyDifferenceValue(0)
, numMelCoefficients(20)
{

    setFrameAndBufferSize(inputFrameSize, analysisBufferSize);
    setSamplingFrequency(fs);
    
    
    
    setSignalEnvelopeSmoothingFactor(0.9);
}


//=====================================================================================
void AudioAnalysisFramework::setSamplingFrequency(int fs)
{
    samplingFrequency = fs;
    
    melFrequencySpectrum.setSamplingFrequency(samplingFrequency);
}

//=====================================================================================
void AudioAnalysisFramework::setShouldCalculateFFT(bool calculateFFT)
{
    shouldCalculateFFFT = calculateFFT;
}

//=====================================================================================
void AudioAnalysisFramework::setNumberOfMelFrequencyCoefficients(int numCoeffs)
{
    numMelCoefficients = numCoeffs;
    
    melFrequencySpectrum.setParameters(audioAnalysisBufferSize, numMelCoefficients);
}

//=====================================================================================
int AudioAnalysisFramework::getSamplingFrequency()
{
    return samplingFrequency;
}

//=====================================================================================
int AudioAnalysisFramework::getInputAudioFrameSize()
{
    return inputAudioFrameSize;
}

//=====================================================================================
int AudioAnalysisFramework::getAudioAnalysisBufferSize()
{
    return audioAnalysisBufferSize;
}

//=====================================================================================
void AudioAnalysisFramework::setFrameAndBufferSize(int inputFrameSize,int analysisBufferSize)
{
    inputAudioFrameSize = inputFrameSize;
    
    audioAnalysisBufferSize = analysisBufferSize;
    
    // !
    // Ensure your input audio frame size is a positive power of two
    assert(isPositivePowerOfTwo(inputAudioFrameSize));
    
    // !
    // Ensure your audio analysis buffer size is a positive power of two
    assert(isPositivePowerOfTwo(audioAnalysisBufferSize));
    
    // !
    // Your input audio frame size should be smaller than or equal to your input audio buffer size
    assert(inputAudioFrameSize <= audioAnalysisBufferSize);
    
    // Set the audio analysis buffer size and fill with zeros
    audioAnalysisBuffer.resize(audioAnalysisBufferSize);
    std::fill(audioAnalysisBuffer.begin(), audioAnalysisBuffer.end(), 0);
    
    // initialise fft
    fft.initialise(audioAnalysisBufferSize);
    
    // set prev magnitude spectrum for spectral flux
    prevMagSpecSpectralFlux.resize(fft.magnitudeSpectrum.size());
    std::fill(prevMagSpecSpectralFlux.begin(), prevMagSpecSpectralFlux.end(), 0);
    
    
    // set buffers for calculating complex spectral difference
    prevMagSpecCSD.resize(fft.magnitudeSpectrum.size());
    prevPhase1CSD.resize(fft.magnitudeSpectrum.size());
    prevPhase2CSD.resize(fft.magnitudeSpectrum.size());

    std::fill(prevMagSpecCSD.begin(), prevMagSpecCSD.end(), 0);
    std::fill(prevPhase1CSD.begin(), prevPhase1CSD.end(), 0);
    std::fill(prevPhase2CSD.begin(), prevPhase2CSD.end(), 0);
    
    melFrequencySpectrum.setParameters(audioAnalysisBufferSize, numMelCoefficients);
 }

//=====================================================================================

void AudioAnalysisFramework::processFrame(std::vector<float> audioFrame)
{
    int numSamples = (int) audioFrame.size();
    
    processFrame(&audioFrame[0], numSamples);
}

/*
// Anki: Unused method
void AudioAnalysisFramework::processInverseFrame(std::vector<float> audioFrame)
{
    //int numSamples = (int) audioFrame.size();
    
    processInverseFrame(&audioFrame[0], numSamples);
}
*/

//=====================================================================================
void AudioAnalysisFramework::processFrame(float* audioFrame,int numSamples)
{
    // !!
    // If this assertion is raised, then you are initialising your instance of
    // the audio analysis class with one frame size and passing it audio frames of
    // a different size (or, at least, indicating that there are a different number of
    // samples to the value given in the initialisation)
    assert(numSamples == inputAudioFrameSize);
    
    // Set flags indicating whether features have been calculated to false
    energyDifferenceCalculated = false;
    rmsCalculated = false;
    
    // Shift back the audio samples in the audio analysis buffer to make
    // room for the new audio frame samples
    for (int i = 0; i < (audioAnalysisBufferSize-inputAudioFrameSize);i++)
    {
        audioAnalysisBuffer[i] = audioAnalysisBuffer[i+inputAudioFrameSize];
    }
    
    // now copy the new audio samples into the audio analysis buffer
    int k = 0;
    for (int i = (audioAnalysisBufferSize-inputAudioFrameSize);i < audioAnalysisBufferSize;i++)
    {
        audioAnalysisBuffer[i] = audioFrame[k];
        k++;
    }
    
    if (shouldCalculateFFFT)
    {
        // calculate the FFT
        fft.calculateFFT(audioAnalysisBuffer.data());
    }
    
}

void AudioAnalysisFramework::processInverseFrame(float* buffer)
{		// calculate the FFT
	fft.calculateIFFT(buffer);
}

//=====================================================================================

float AudioAnalysisFramework::getPeakEnergy()
{
    float maxVal = 0.0;
    
    for (int i = 0;i < audioAnalysisBuffer.size();i++)
    {
        float value = fabsf(audioAnalysisBuffer[i]);
        
        if (value > maxVal)
        {
            maxVal = value;
        }
    }

    return maxVal;
}

//=====================================================================================
float AudioAnalysisFramework::getRMS()
{
    if (rmsCalculated)
    {
        return rmsValue;
    }
    else
    {
        float sumOfSquares = 0;
        
        for (int i = 0;i < audioAnalysisBuffer.size();i++)
        {
            sumOfSquares += audioAnalysisBuffer[i]*audioAnalysisBuffer[i];
        }
        
        float meanValue = sumOfSquares / (float) audioAnalysisBufferSize;
        
        rmsValue = sqrtf(meanValue);
        
        rmsCalculated = true;
        
        return rmsValue;
    }
        
}

//=====================================================================================
void AudioAnalysisFramework::setSignalEnvelopeSmoothingFactor(float smoothingFactor)
{
    // !
    // The smoothing factor should be between 0.0 and 1.0
    assert(smoothingFactor < 1.0);
    assert(smoothingFactor >= 0.0);
    
    float cutoff = 0.5 - (smoothingFactor / 2.);
    
    lowpassFilter.configure(cutoff);
}

//=====================================================================================
float AudioAnalysisFramework::getSignalEnvelope()
{
    float rmsValue = getRMS();
    
    return fmaxf(0,lowpassFilter.processSample(rmsValue));
}

//=====================================================================================
float AudioAnalysisFramework::getEnergyDifference()
{
    if (energyDifferenceCalculated)
    {
        return energyDifferenceValue;
    }
    else
    {
        float energyValue = getRMS();
        
        energyDifferenceValue = fabsf(energyValue - previousEnergyDifferenceValue);
        
        previousEnergyDifferenceValue = energyValue;
        
        energyDifferenceCalculated = true;
        
        return  energyDifferenceValue;
    }
}

//=====================================================================================
void AudioAnalysisFramework::checkFFTHasBeenCalculated()
{
    // !
    // You have told the instance not to calculate the FFT, but you are
    // still requiring the instance to produce features that require the FFT
    assert(shouldCalculateFFFT == true);
}

//=====================================================================================
std::vector<float> AudioAnalysisFramework::getMagnitudeSpectrum()
{
    checkFFTHasBeenCalculated();
    
    return fft.magnitudeSpectrum;
}

//=====================================================================================
std::vector<float> AudioAnalysisFramework::getRealFFT() // Added by Foivos
{
    checkFFTHasBeenCalculated();
    
    return fft.real;
}

std::vector<float> AudioAnalysisFramework::getImagFFT() // Added by Foivos
{
	checkFFTHasBeenCalculated();

	return fft.imag;
}

//=====================================================================================
std::vector<float> AudioAnalysisFramework::getMelSpectrum()
{
    checkFFTHasBeenCalculated();
    
    return melFrequencySpectrum.calculateMelSpectrum(fft.magnitudeSpectrum);
}

//=====================================================================================
float AudioAnalysisFramework::getSpectralCentroid()
{
    checkFFTHasBeenCalculated();
    
    float resolution = (float) samplingFrequency / (float) audioAnalysisBufferSize;
    
    float weightedSum = 0;
    float sum = 0;
    
    for (int i = 0;i < fft.magnitudeSpectrum.size();i++)
    {
        float centreFrequency = resolution * (float) i;
        
        weightedSum += centreFrequency * fft.magnitudeSpectrum[i];
        
        sum += fft.magnitudeSpectrum[i];
    }
    
    if (sum == 0.0)
    {
        return 0.0;
    }
    else
    {
        return weightedSum / sum;
    }
}

//=====================================================================================
float AudioAnalysisFramework::getSpectralFlatness()
{
    checkFFTHasBeenCalculated();
    
    float logSum = 0;
    float sum = 0;
    
    for (int i = 0;i < fft.magnitudeSpectrum.size();i++)
    {
        // we add 1 here to stop zeros making the logs -inf
        float sample = 1 + fft.magnitudeSpectrum[i];
        
        logSum += logf(sample);
        sum += sample;
    }
    
    logSum = logSum / (float) fft.magnitudeSpectrum.size();
    sum = sum / (float) fft.magnitudeSpectrum.size();
    
    logSum = expf(logSum);
    
    if (sum == 0)
    {
        return 0.0;
    }
    else
    {
        return logSum / sum;
    }
}

//=====================================================================================
float AudioAnalysisFramework::getSpectralRolloff()
{
    checkFFTHasBeenCalculated();
    
    // sum the spectrum
    float spectralSum = 0;
    
    for (int i = 0;i < fft.magnitudeSpectrum.size();i++)
    {
        spectralSum += fft.magnitudeSpectrum[i];
    }
    
    // calculate the rolloff threshold;
    float rolloffThreshold = spectralSum*0.85;
    
    // define the rolloff index
    float rolloffIndex = 0;
    
    // reset the spectral sum
    spectralSum = 0;
    
    for (int i = 0;i < fft.magnitudeSpectrum.size();i++)
    {
        // sum the spectrum again
        spectralSum += fft.magnitudeSpectrum[i];
        
        // if we exceed the rolloff threshold
        if (spectralSum > rolloffThreshold)
        {
            // store the index and break
            rolloffIndex = (float)i;
            break;
        }
    }
    
    // calclulate spectral rolloff normalised by the length of the magnitude spectrum
    float spectralRolloff = rolloffIndex / (float) fft.magnitudeSpectrum.size();
    
    return spectralRolloff;
}

//=====================================================================================
float AudioAnalysisFramework::getSpectralFlux()
{
    checkFFTHasBeenCalculated();
    
    float sum = 0;
    
    for (int i = 0;i < fft.magnitudeSpectrum.size();i++)
    {
        float difference = fabs(fft.magnitudeSpectrum[i] - prevMagSpecSpectralFlux[i]);
        
        sum += difference;
        
        prevMagSpecSpectralFlux[i] = fft.magnitudeSpectrum[i];
    }
    
    return sum;
}

//=====================================================================================
float AudioAnalysisFramework::getHighFreqencyContentOnsetDectionFunction()
{
    checkFFTHasBeenCalculated();
    
    float sum = 0;
    
    for (int i = 0;i < fft.magnitudeSpectrum.size();i++)
    {
        sum += (fft.magnitudeSpectrum[i]* (float) i);
    }
    
    return sum;
}

//=====================================================================================
float AudioAnalysisFramework::getComplexSpectralDifference()
{
    checkFFTHasBeenCalculated();
    
    float phaseDeviation;
    float complexSpectralDifference;
    
    complexSpectralDifference = 0;
    
    for (int i = 0;i < fft.magnitudeSpectrum.size();i++)
    {
        float phaseValue = atan2(fft.imag[i],fft.real[i]);
        
        phaseDeviation = phaseValue - (2*prevPhase1CSD[i]) + prevPhase2CSD[i];
        
        phaseDeviation = phaseWrap(phaseDeviation);
        
        float magnitudeDifference = fft.magnitudeSpectrum[i] - prevMagSpecCSD[i];
        
        float phaseDifference = -fft.magnitudeSpectrum[i]*sin(phaseDeviation);
        
        complexSpectralDifference += sqrt((magnitudeDifference*magnitudeDifference) + (phaseDifference*phaseDifference));
        
        prevPhase2CSD[i] = prevPhase1CSD[i];
        prevPhase1CSD[i] = phaseValue;
        prevMagSpecCSD[i] = fft.magnitudeSpectrum[i];
    }
    
    return complexSpectralDifference;
}

//=====================================================================================
float AudioAnalysisFramework::calculateSpectralMoment(int n)
{
    checkFFTHasBeenCalculated();
    
    float denominatorSum = 0;
    float numeratorSum = 0;
    
    for (int i = 0;i < fft.magnitudeSpectrum.size();i++)
    {
        float k = (float) i;
        numeratorSum += powf(k, n) * fft.magnitudeSpectrum[i];
        
        denominatorSum += fft.magnitudeSpectrum[i];
    }
    
    if (denominatorSum == 0)
    {
        return 0.0;
    }
    else
    {
        return numeratorSum / denominatorSum;
    }
}

//=====================================================================================
float AudioAnalysisFramework::getSpectralKurtosis()
{
    float u1 = calculateSpectralMoment(1);
    float u2 = calculateSpectralMoment(2);
    float u3 = calculateSpectralMoment(3);
    float u4 = calculateSpectralMoment(4);
    
    float spread = sqrt(u2 - u1*u1);
    
    float x = -3*powf(u1, 4) + 6*u1*u2 + 4*u1*u3 + u4;
    float y = powf(spread, 4);
    
    float kurtosis;
    
    if (y == 0)
    {
        kurtosis = 0.0;
    }
    else
    {
        kurtosis = (x / y) - 3;
    }
    
    
    
    return kurtosis;
}
