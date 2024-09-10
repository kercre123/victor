//=====================================================================================
/**
 * Copyright Krotos LTD 2015
 *
 * All rights reserved.
 */
//=====================================================================================

#ifndef __Krotos__MelFrequencySpectrum__
#define __Krotos__MelFrequencySpectrum__

#include <vector>
#include <math.h>
#include <iostream>
#include <assert.h>


//=====================================================================================
class MelFrequencySpectrum
{
    
public:
    
    /** Constructor */
    MelFrequencySpectrum();
    
    /** Sets the sampling frequency 
     * @param fs the sampling frequency 
     */
    void setSamplingFrequency(int fs);
    
    /** Sets the audio frame size and number of Mel-frequency coefficients */
    void setParameters(int audioFrameSize,int numCoeffs);
    
    /** Calcualtes the mel spectrum from the magnitude spectrum given */
    std::vector<float> calculateMelSpectrum(std::vector<float> magnitudeSpectrum);
    
    /** Builds the Mel filter bank */
    void buildMelFilterbank();
    
private:
    
    static float hertzToMel(float f);
    static float melToHertz(float m);
    
    int numCoefficients;
    int fftSize;
    int magnitudeSpectrumSize;
    int samplingFrequency;
    
    std::vector<std::vector<float> > melFilterBank;
};

#endif /* defined(__Krotos_Audio_Analysis_Test__MelFrequencySpectrum__) */
