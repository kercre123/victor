#include "KrotosVocoderFXDSP.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkSimd.h>
#include <AK/Plugin/PluginServices/AkVectorValueRamp.h>
#include <math.h>

#include "KrotosVectorOperations.h"

CKrotosVocoderFXDSP::CKrotosVocoderFXDSP( ) 
{

}

CKrotosVocoderFXDSP::~CKrotosVocoderFXDSP( )
{
	if (vocoder)
	{
		AK_PLUGIN_DELETE(m_pAllocator, vocoder);
		vocoder = NULL;
	}
}

void CKrotosVocoderFXDSP::Setup( KrotosVocoderFXParams* /*pInitialParams*/, bool in_bIsSendMode, AkUInt32 in_uSampleRate, AK::IAkPluginMemAlloc* in_pAllocator)
{
    m_bSendMode = in_bIsSendMode;
    m_PreviousParams.Reset();
    m_uSampleRate = in_uSampleRate;
	m_pAllocator = in_pAllocator;

	m_MaxExtraSamples = m_uSampleRate * 5;	// For safety, define the maximum length we will allow the tail to be extended

	//AKPLATFORM::OutputDebugMsg("allocating vocoder \n");
	vocoder = AK_PLUGIN_NEW(m_pAllocator, Vocoder(m_pAllocator));
	if (vocoder)
	{
		//AKPLATFORM::OutputDebugMsg("vocoder prepare to play \n");
		vocoder->prepareToPlay(in_uSampleRate, m_pAllocator);
	}
}

// The effect should process all the input data (uValidFrames) as long as AK_DataReady is passed in the eState field. 
// When the input is finished (AK_NoMoreData), the effect can output more sample than uValidFrames up to MaxFrames() if desired. 
// All sample frames beyond uValidFrames are not initialized and it is the responsibility of the effect to do so when outputting an effect tail.
// The effect must notify the pipeline by updating uValidFrames if more frames are produced during the effect tail.
// The effect will stop being called by the pipeline when AK_NoMoreData is returned in the the eState field of the AkAudioBuffer structure.

//const float  SilenceThreshold_dB{ 6.30957344480193e-8f };      // -144 dB
//const float  SilenceThreshold_dB{ 0.003981071705534973 };    // -48 dB
const float  SilenceThreshold_dB{ 0.00001584893192461114f };  // -96 dB

void CKrotosVocoderFXDSP::Process( AkAudioBuffer* io_pBuffer, KrotosVocoderFXParams* pCurrentParams )
{
    UpdateVocoderParameters(*pCurrentParams, m_PreviousParams);

    // Make AudioSampleBuffer from AkAudioBuffer for Vocoder

    float* channelFrames[AK_STANDARD_MAX_NUM_CHANNELS];
    const AkUInt32 uNumChannels = io_pBuffer->NumChannels();
    AKASSERT(uNumChannels <= AK_STANDARD_MAX_NUM_CHANNELS);
    for ( AkUInt32 i = 0; i < uNumChannels; ++i )
    {
        channelFrames[i] = (float*) io_pBuffer->GetChannel(i);
    }   

	AkUInt16 uNumFrames = io_pBuffer->uValidFrames;
	vector<float> buffer(io_pBuffer->MaxFrames(), 0);

	// === Deal with FX Tail ===
	if ((io_pBuffer->eState == AK_NoMoreData) && (m_MaxExtraSamples > 0) && (m_outRMS > SilenceThreshold_dB)) // If no more data coming from input, and we were still measuring some RMS
	{																							// during previously RMS measured buffer, then we have a tail to deal with
		AkUInt16 uExtraFrames = io_pBuffer->MaxFrames() - uNumFrames;							// How many extra frames can we fit into the supplied buffer?
		
																								// Must do this before using buffer
		FloatVectorOperations::copy(buffer.data(), channelFrames[0], uNumFrames);

		if (uNumChannels >= 2)
		{
			// Downmix multichannel input to mono
			//float gain = 1.0f / static_cast<float>(uNumChannels);

			for (unsigned int i = 1; i < uNumChannels; i++)
			{
				FloatVectorOperations::add(buffer.data(), channelFrames[i], uNumFrames);
			}

		}
		uNumFrames += uExtraFrames;																// Update the number of frames we will return
		io_pBuffer->uValidFrames = uNumFrames;													// and inform Ak how many to expect and
		io_pBuffer->eState = AK_DataReady;														// that we are not finished.
		m_MaxExtraSamples -= uNumFrames;														// For safety we count down a max no of extra samples (Frames in Ak speak).
	}
	else
	{
		FloatVectorOperations::copy(buffer.data(), channelFrames[0], uNumFrames);

		if (uNumChannels >= 2)
		{
			// Downmix multichannel input to mono
			//float gain = 1.0f / static_cast<float>(uNumChannels);

			for (unsigned int i = 1; i < uNumChannels; i++)
			{
				FloatVectorOperations::add(buffer.data(), channelFrames[i], uNumFrames);
			}

		}
	}
	   

	//volume adjustments for carrier waveform
	float volAdjust = 0;

	switch (pCurrentParams->iCarrierWaveform)
	{	
	case 8: //tangent
		volAdjust = 16.0f;
		break;
	default:
		volAdjust = 0.f;
		break;
	}

	float volAdjustLevel = powf(10.0f, volAdjust * 0.05f);
	
	float wetLevel = powf(10.f, (pCurrentParams->wetVol) * 0.05f);
	float dryLevel = powf(10.f, (pCurrentParams->dryVol) * 0.05f);

	vector<float> dryBuffer(uNumFrames, 0);
	FloatVectorOperations::copy(dryBuffer.data(), buffer.data(), uNumFrames);

    vocoder->processBlock(buffer);

	{
		double sum = 0.0;

		for (int i = 0; i < buffer.size(); ++i)
		{
			const float sample = buffer[i];
			sum += sample * sample;
		}
		m_outRMS = (float)sqrt(sum / buffer.size());
	}

	//apply volumes
	applyGain(dryBuffer, dryLevel);
	applyGain(buffer, volAdjustLevel);
	applyGain(buffer, wetLevel);

	FloatVectorOperations::add(buffer.data(), dryBuffer.data(), uNumFrames);
	
	float gain = 1.0f / static_cast<float>(uNumChannels);
	applyGain(buffer, gain);

	//copy back to the wwisebuffer
	FloatVectorOperations::copy(channelFrames[0], buffer.data(), uNumFrames);
	if (uNumChannels >= 2)
	{
		// Upmix mono to multichannel output
		for (unsigned int i = 1; i < uNumChannels; i++)
		{
			FloatVectorOperations::copy(channelFrames[i], buffer.data(), uNumFrames);
		}
	}

    // Parameter ramps are reached
    m_PreviousParams = *pCurrentParams;
}

void CKrotosVocoderFXDSP::applyGain(vector<float> &buffer, float gain)
{
	if (gain != 1.0f)
	{
		if (gain == 0.0f)
		{
			buffer.clear();
		}
		else
		{
			FloatVectorOperations::multiply(buffer.data(), gain, static_cast<int>(buffer.size()));
		}
	}
}

void CKrotosVocoderFXDSP::UpdateVocoderParameters(KrotosVocoderFXParams& current, KrotosVocoderFXParams& previous)
{
    // Update carrier mode
    if (current.iCarrier != previous.iCarrier)
    {
        vocoder->setCarrier((Vocoder::Carrier)current.iCarrier);
    }

    // Update carrier frequency only if set by UI mode
    if ((AkUInt32)Vocoder::Carrier::OscillatorFrequencySetByUI == current.iCarrier)
    {
        vocoder->setCarrierFrequency(current.fCarrierFrequency);
    }

    // Update carrier waveform changes if not noise mode
    if ((current.iCarrierWaveform != previous.iCarrierWaveform)
        && ((AkUInt32)Vocoder::Carrier::Noise != current.iCarrier))
    {
        vocoder->setCarrierWaveform((WaveShapes)current.iCarrierWaveform);
    }

    // Update frequency min & max only if pitch tracking
    if ((AkUInt32)Vocoder::Carrier::OscillatorPitchTracking == current.iCarrier)
    {
        vocoder->setCarrierFrequencyMax(current.fCarrierFrequencyMax);
        vocoder->setCarrierFrequencyMin(current.fCarrierFrequencyMin);
    }

    // Update envelope attack changes
    if (current.fEnvelopeAttackTime != previous.fEnvelopeAttackTime)
    {
        vocoder->setEnvelopAttackTime(current.fEnvelopeAttackTime / 1000.0f);
    }

    // Update envelope release changes
    if (current.fEnvelopeReleaseTime != previous.fEnvelopeReleaseTime)
    {
        vocoder->setEnvelopReleaseTime(current.fEnvelopeReleaseTime / 1000.0f);
    }

    // Update tracker algorithm changes
    if (current.iPitchTrackerAlgorithm != previous.iPitchTrackerAlgorithm)
    {
        vocoder->setPitchTrackerAlgorithm((PitchTracker::Algorithm)current.iPitchTrackerAlgorithm);
    }

    // Update EQ changes
    for (int i = 0; i < 8 /*numberOfFftGains*/; i++)
    {
        if (current.fEQSlider[i] != previous.fEQSlider[i])
            vocoder->setGainBin(current.fEQSlider[i], i, 8 /*numberOfFftGains*/);
    }

/*	if (current.wetVol != previous.wetVol)
	{
		vocoder.setWetVol(current.wetVol);
	}

	if (current.dryVol != previous.dryVol)
	{
		vocoder.setDryVol(current.dryVol);
	}*/
}
