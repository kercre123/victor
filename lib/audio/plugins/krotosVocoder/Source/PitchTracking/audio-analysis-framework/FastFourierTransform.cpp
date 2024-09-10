//=====================================================================================
/**
 * Copyright Krotos LTD 2015
 *
 * All rights reserved.
 */
//=====================================================================================

#include "FastFourierTransform.h"

#ifdef FFT_IMPLEMENTATION_APPLE
#define AUDIOFFT_APPLE_ACCELERATE
#endif

//=====================================================================================
FastFourierTransform::FastFourierTransform() : fftInitialised(false)
{
    
#if defined(FFT_IMPLEMENTATION_KISS_FFT)
    // all good
#else
    // !
    // You have not indicated which FFT implementation you wish to use
    assert(false);
#endif
    
}

//=====================================================================================
FastFourierTransform::~FastFourierTransform()
{
    if (fftInitialised)
    {
        freeFFTConfiguration();
    }
}

//=====================================================================================
void FastFourierTransform::initialise(const int frameSize)
{    
    if (fftInitialised)
    {
        freeFFTConfiguration();
    }
    
    audioFrameSize = frameSize;
    
    magnitudeSpectrum.resize((frameSize/2)+1);
    
#ifdef FFT_IMPLEMENTATION_KISS_FFT
    real.resize(frameSize);
    imag.resize(frameSize);

	fftrConfig = kiss_fftr_alloc(1024, false, 0, 0);
	inverseFftrConfig = kiss_fftr_alloc(1024, true, 0, 0);
    fftInitialised = true;
#endif

}

//=====================================================================================
void FastFourierTransform::calculateFFT(float* audioFrame)
{
	// !
	// If this assertion is raised, then you are sending audio frames of
	// a different size to the size that you initialised the FFT object with...
	//assert(audioFrame.size() == audioFrameSize);

#ifdef FFT_IMPLEMENTATION_KISS_FFT 

	/*for (int i = 0; i < audioFrameSize; i++)
	{
		input_forward[i] = audioFrame[i];
	}*/

	kiss_fftr(fftrConfig, audioFrame, (kiss_fft_cpx*)audioFrame);
#endif

#if defined(FFT_IMPLEMENTATION_OOURA) || defined(FFT_IMPLEMENTATION_APPLE)
	fft.fft(audioFrame.data(), real.data(), imag.data());

	for (int i = 0; i < magnitudeSpectrum.size(); i++)
	{
		magnitudeSpectrum[i] = sqrt((real[i] * real[i]) + (imag[i] * imag[i]));
	}
#endif
}

void FastFourierTransform::testFFT(float* audio, int numSamples)
{
	/*for (int i = 0; i < numSamples; i++)
	{
		input_forward[i] = audio[i];
	}

	kiss_fftr(fftrConfig, input_forward, output_forward);

	float scale = 1 / (float)numSamples;
	for (int i = 0; i < numSamples; i++)
	{
		output_forward[i].r *= scale;
		output_forward[i].i *= scale;
	}

	kiss_fftri(inverseFftrConfig, output_forward, outputInverse);

	for (int i = 0; i < numSamples; i++)
	{
		audio[i] = outputInverse[i];
	}*/
}

void FastFourierTransform::calculateIFFT(float* buffer)
{
	// !
	// If this assertion is raised, then you are sending audio frames of
	// a different size to the size that you initialised the FFT object with...
	//assert(audioFrame.size() == audioFrameSize);

#ifdef FFT_IMPLEMENTATION_KISS_FFT
	kiss_fftri(inverseFftrConfig, (kiss_fft_cpx*)buffer, buffer);
#endif

#if defined(FFT_IMPLEMENTATION_OOURA) || defined(FFT_IMPLEMENTATION_APPLE)
	fft.ifft(output, real, imag);
#endif
}

//=====================================================================================
void FastFourierTransform::freeFFTConfiguration()
{
#ifdef FFT_IMPLEMENTATION_KISS_FFT
	free(fftrConfig);
	free(inverseFftrConfig);
#endif
    
    fftInitialised = false;
}
