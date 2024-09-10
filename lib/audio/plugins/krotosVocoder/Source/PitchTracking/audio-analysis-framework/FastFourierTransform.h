//=====================================================================================
/**
 * Copyright Krotos LTD 2015
 *
 * All rights reserved.
 */
//=====================================================================================


#ifndef __Krotos__FastFourierTransform__
#define __Krotos__FastFourierTransform__

#include <vector>
#include <cmath>
#include <assert.h>

#define FFT_IMPLEMENTATION_KISS_FFT

//=====================================================================================
#ifdef FFT_IMPLEMENTATION_KISS_FFT
#include "libraries/kiss_fft/kiss_fft.h"
#include "libraries/kiss_fft/kiss_fftr.h"
#endif


//=====================================================================================
class FastFourierTransform
{
public:
    
    //=====================================================================================
    /** Constructor */
    FastFourierTransform();

    /** Destructor */
    ~FastFourierTransform();
    
    //=====================================================================================
    /** Initialise the FFT object with the frame size that will be used to calculate FFTs 
     * @param frameSize the frame size to be used
     */
    void initialise(const int frameSize);
    
    /** Calculates the FFT on the audio frame passed to it. The result is stored in the public variables
     * of the class - @see magnitudeSpectrum, real, imag
     * @param audioFrame the audio frame from which the FFT will be calculated. The size of this audio frame
     * must match the frame size that was used to initialise the FFT object.
     */
	void calculateFFT(float* audioFrame);
	void calculateIFFT(float* buffer);

	void testFFT(float* audio, int numSamples);
    
    /** Frees any memory used to calculate the FFT, if necessary */
    void freeFFTConfiguration();
    
    //=====================================================================================
    /** Holds the magnitude spectrum after the calculation of the FFT */
    std::vector<float> magnitudeSpectrum;
    
    /** Holds the real valued result of the FFT */
    std::vector<float> real;
    
    /** Holds the imaginary valued result of the FFT */
    std::vector<float> imag;
    
private:
    
    bool fftInitialised;
    int audioFrameSize;
    
#ifdef FFT_IMPLEMENTATION_KISS_FFT

	kiss_fftr_cfg fftrConfig;
	kiss_fftr_cfg inverseFftrConfig;
#endif
    
    
#if defined(FFT_IMPLEMENTATION_OOURA) || defined(FFT_IMPLEMENTATION_APPLE)
    audiofft::AudioFFT fft;
#endif
    
    
};

#endif /* defined(__Krotos__FastFourierTransform__) */
