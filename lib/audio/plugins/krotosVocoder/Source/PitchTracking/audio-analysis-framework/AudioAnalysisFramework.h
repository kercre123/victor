//=====================================================================================
/**
 * Copyright Krotos LTD 2015
 *
 * All rights reserved.
 */
//=====================================================================================

#ifndef __Krotos__AudioAnalysisFramework__
#define __Krotos__AudioAnalysisFramework__

#include <vector>
#include <cmath>
#include "assert.h"
#include "FastFourierTransform.h"
#include "LowpassFilter.h"
#include "MelFrequencySpectrum.h"

//=====================================================================================
class AudioAnalysisFramework
{
public:
    
    //=====================================================================================
    /** Constructor. Initialises the audio analysis framework with the input audio frame size,
     * the audio analysis buffer size and the sampling frequency. The input audio frame size and
     * audio analysis buffer size must be powers of two, and the audio analysis buffer size must 
     * be greater than or equal to the input audio frame size.
     * @param inputFrameSize the input audio frame size
     * @param analysisBufferSize the audio analysis buffer size
     * @param fs the sampling frequency
     */
    AudioAnalysisFramework(int inputFrameSize, int analysisBufferSize,int fs);
    
    //=====================================================================================
    // Mutators
    
    /** Sets the sampling frequency to be used for audio analysis 
     * @param fs the sampling frequency to be used
     */
    void setSamplingFrequency(int fs);
    
    /** Sets the input audio frame size and audio analysis buffer size. Both these values
     * must be powers of two, and the audio analysis buffer size must be greater than or
     * equal to the input audio frame size
     * @param inputFrameSize the input audio frame size
     * @param analysisBufferSize the audio analysis buffer size
     */
    void setFrameAndBufferSize(int inputFrameSize,int analysisBufferSize);
    
    /** You can ask the instance not to calculate the FFT, in the case where you don't
     * need any of the FFT based features. The instance will calculate the FFT by default
     * and definitely don't tell it not to if you need any spectral features or the 
     * magnitude spectrum 
     * @param calculateFFT a boolean indicating whether or not you wish to use the FFT
     */
    void setShouldCalculateFFT(bool calculateFFT);
    
    /** Sets the smoothing factor for the signal envelope. 
     * @param smoothingFactor the smoothing factor, where 1 is the most filtered, 0 is no smoothing at all
     */
    void setSignalEnvelopeSmoothingFactor(float smoothingFactor);
    
    /** Sets the number of Mel-frequency Coefficients to be used for the Mel-frequency Spectrum */
    void setNumberOfMelFrequencyCoefficients(int numCoeffs);
    
    //=====================================================================================
    // Accessors
    
    /** @Returns the sampling frequency */
    int getSamplingFrequency();
    
    /** @Returns the input audio frame size */
    int getInputAudioFrameSize();
    
    /** @Returns the audio analysis buffer size */
    int getAudioAnalysisBufferSize();
    
    //=====================================================================================
    // Audio frame processing
    
    /** Passes a single audio frame to the library for processing in vector format. The size of
     * the vector must match the input audio frame size used to initialise the class. After
     * this, audio analysis features can be requested using other methods (e.g. getPeakEnergy())
     * @param audioFrame a vector containing the samples of an input audio frame
     */
    void processFrame(std::vector<float> audioFrame);

    // void processInverseFrame(std::vector<float> audioFrame); // Anki: Unused
    
    /** Passes a single audio frame to the library for processing in array format, with an. 
     * accompanying number of samples (which must match the input audio frame size that was used
     * to initialised the class. After this, audio analysis features can be requested using other 
     * methods (e.g. getPeakEnergy())
     * @param audioFrame a pointer to an array containing the samples of an input audio frame
     * @param numSamples the number of samples in the array
     */
    void processFrame(float* audioFrame,int numSamples);

	void processInverseFrame(float* buffer);
    
    //=====================================================================================
    // Energy based methods
    
    /** @Returns the peak energy of the audio analysis buffer */
    float getPeakEnergy();
    
    /** @Returns the RMS energy of the audio analysis buffer */
    float getRMS();
    
    /** @Returns the signal envelope */
    float getSignalEnvelope();
    
    /** @Returns the change in energy over time */
    float getEnergyDifference();
    
    //=====================================================================================
    // Spectra
    
    /** @Returns the magnitude spectrum as a vector */
    std::vector<float> getMagnitudeSpectrum();
    
    //** @Returns the real part of the spectrum as a vector (added by Foivos)*/
    std::vector<float> getRealFFT();

	std::vector<float> getImagFFT();

	float* getFFTComplex();
    
    /** @Returns the mel spectrum as a vector */
    std::vector<float> getMelSpectrum();
    
    //=====================================================================================
    // Frequency-domain features
    
    /** @Returns the spectral centroid */
    float getSpectralCentroid();
    
    /** @Returns the spectral flatness */
    float getSpectralFlatness();
    
    /** @Returns the spectral rolloff */
    float getSpectralRolloff();
    
    /** @Returns the spectral flux of the signal */
    float getSpectralFlux();
    
    /** @Returns the spectral kurtosis of the signal */
    float getSpectralKurtosis();
    
    /** @Returns the high frequency content onset detection function */
    float getHighFreqencyContentOnsetDectionFunction();
    
    /** @Returns the complex spectral difference onset detection function */
    float getComplexSpectralDifference();
    
private:
    
    void checkFFTHasBeenCalculated();
        
    float calculateSpectralMoment(int n);
    
    int samplingFrequency;
    int inputAudioFrameSize;
    int audioAnalysisBufferSize;
    
    std::vector<float> audioAnalysisBuffer;
    
    std::vector<float> prevMagSpecSpectralFlux;
    
    bool shouldCalculateFFFT;
    
    FastFourierTransform fft;
    
    bool rmsCalculated;
    float rmsValue;
    
    bool energyDifferenceCalculated;
    float energyDifferenceValue;
    float previousEnergyDifferenceValue;
    
    std::vector<float> prevMagSpecCSD;
    std::vector<float> prevPhase1CSD;
    std::vector<float> prevPhase2CSD;
        
    LowpassFilter lowpassFilter;
    MelFrequencySpectrum melFrequencySpectrum;
    
    int numMelCoefficients;
    
};

#endif /* defined(__Krotos__AudioAnalysisFramework__) */
