//=====================================================================================
/**
 * Copyright Krotos LTD 2015
 *
 * All rights reserved.
 */
//=====================================================================================

#include "MelFrequencySpectrum.h"

//=====================================================================================
MelFrequencySpectrum::MelFrequencySpectrum()
{
    fftSize = 512;
    magnitudeSpectrumSize = 257;
    numCoefficients = 20;
    samplingFrequency = 44100;
    
    setParameters(fftSize, numCoefficients);
    setSamplingFrequency(samplingFrequency);
    
}

//=====================================================================================
float MelFrequencySpectrum::hertzToMel(float f)
{
    return 1127 * log(1 + (f / 700.f));
}

//=====================================================================================
float MelFrequencySpectrum::melToHertz(float m)
{
    return 700.f * (exp(m / 1127.f) - 1.f);
}


//=====================================================================================
void MelFrequencySpectrum::setParameters(int audioFrameSize,int numCoeffs)
{
    fftSize = audioFrameSize;
    magnitudeSpectrumSize = (audioFrameSize / 2) + 1;
    numCoefficients = numCoeffs;
    
    buildMelFilterbank();
}

//=====================================================================================
void MelFrequencySpectrum::setSamplingFrequency(int fs)
{
    samplingFrequency = fs;
    
    buildMelFilterbank();
}

//=====================================================================================
std::vector<float> MelFrequencySpectrum::calculateMelSpectrum(std::vector<float> magnitudeSpectrum)
{
    // !
    // We are expecting to receive a magnitude spectrum with a size that is
    // (N / 2) + 1, where N is the audioFrameSize that you initialised the Mel-spectrum
    // object with. This isn't the case and so it is being initialised wrongly.
    assert(magnitudeSpectrum.size() == magnitudeSpectrumSize);
    
    std::vector<float> melSpectrum;
    
    for (int m = 0;m < numCoefficients;m++)
    {
        float melValue = 0;
        
        for (int k = 0;k < magnitudeSpectrum.size();k++)
        {
            melValue += (magnitudeSpectrum[k]*magnitudeSpectrum[k])*melFilterBank[m][k];
        }
        
        melSpectrum.push_back(melValue);
    }
    
    return melSpectrum;
}

//=====================================================================================
void MelFrequencySpectrum::buildMelFilterbank()
{
    float lowFreq = 20;
    float highFreq = ((float)samplingFrequency) / 2.f;
    
    float lowMelFrequency = hertzToMel(lowFreq);
    float highMelFrequency = hertzToMel(highFreq);
    
    melFilterBank.resize(magnitudeSpectrumSize);
    
    for (int i = 0;i < numCoefficients;i++)
    {
        melFilterBank[i].resize(magnitudeSpectrumSize);
        std::fill(melFilterBank[i].begin(), melFilterBank[i].end(), 0);
    }
    
    
    
    std::vector<float> melPoints;
    
    float melRange = highMelFrequency - lowMelFrequency;
    
    float stepSize = melRange / ((float)numCoefficients + 1);
    
    melPoints.push_back(lowMelFrequency);
    for (int i = 0;i < numCoefficients;i++)
    {
        melPoints.push_back(lowMelFrequency + ((i+1)*stepSize));
    }
    melPoints.push_back(highMelFrequency);
    

    std::vector<int> melFilterBins;
    
    for (int i = 0;i < melPoints.size();i++)
    {
        float h = melToHertz(melPoints[i]);
        
        int k = ((float)fftSize) * (h / ((float) samplingFrequency));
        
        melFilterBins.push_back(k);
    }
    
    for (int m = 0;m < numCoefficients;m++)
    {
        // !
        // If you are getting this assertion failure, then you are trying to get
        // too many separate Mel-frequency bins from a FFT magnitude spectrum that isn't
        // large enough to give you the separation you need. Either increase your
        // FFT size or set the number of Mel-frequency coefficients to a smaller number
        assert(melFilterBins[m] < melFilterBins[m+1]);
        
        for (int k = 0;k < magnitudeSpectrumSize;k++)
        {
            if ((k >= melFilterBins[m]) && (k <= melFilterBins[m+1]))
            {
                float t1 = (float) (k - melFilterBins[m]);
                float t2 = (float) (melFilterBins[m+1] - melFilterBins[m]);
                
                melFilterBank[m][k] = t1 / t2;
            }
            else if ((k >= melFilterBins[m+1]) && (k <= melFilterBins[m+2]))
            {
                float t1 = (float) (melFilterBins[m+2] - k);
                float t2 = (float) (melFilterBins[m+2] - melFilterBins[m+1]);
                
                melFilterBank[m][k] = t1 / t2;
            }
            else
            {
                melFilterBank[m][k] = 0.0;
            }
        }
    }
}