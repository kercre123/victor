/*
==============================================================================

Vocoder.cpp
Created: 09 February 2017 12:36:11pm
Author:  Mathieu Carre

==============================================================================
*/

#include "Vocoder.h"

#ifdef AUDIOKINETIC
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#endif

#include "KrotosVectorOperations.h"



Vocoder::Vocoder(AK::IAkPluginMemAlloc* in_pAllocator)
: m_fftNormalizationCoefficient(1.0f / (static_cast<double>(m_analysisWindowSize*64)))
, m_analysisFIFO(m_analysisBufferSize)	
, m_oscillatorFIFO(m_analysisBufferSize)
, m_synthesisFIFO(m_synthesisBufferSize)
{
    m_analysisFIFO.clear(0.0f);
    m_synthesisFIFO.clear(0.0f);

    //Windows
#ifdef GENERATEWINDOW
	Window<float>::generateWindow(m_analysisWindow,  m_analysisWindowSize,  analysisWindowType);
    Window<float>::generateWindow(m_synthesisWindow, m_synthesisWindowSize, synthesisWindowType);
#else
	m_analysisWindow = m_analysis;
	m_synthesisWindow = m_synthesis;
#endif
    float overlappingRatio = 1.0f / static_cast<float>(m_overlappingCoefficient);

    float overlappingWindowCoefficient = Window<float>::calculateOverlappingNormalizationCoefficient(synthesisWindowType, overlappingRatio);

    for (int i = 0; i < m_synthesisWindowSize; i++)
    {
        m_synthesisWindow[i] *= overlappingWindowCoefficient;
    }

    //Oscillators
    m_oscillator.setWaveShape(m_oscillatorShapeInit);
    m_oscillator.setInterpolationToUse(m_oscillatorInterpolation);

    noiseOscillator.setRange(m_noiseOscillatorMin, m_noiseOscillatorMax);

    //Pitch tracker
    m_pitchTracker.setAlgorithm(m_pitchTrackerAlgorithm);
    m_pitchInterpolator.setValue(m_oscillatorFrequencyInit);
    m_pitchTrackerBuffer.resize(m_pitchTrackerBufferSize);

    calculateEqualTemperamentNoteFrequencies(m_equalTemperamentNoteFrequencies);

    //EQ
    FloatVectorOperations::fill(m_eqGain, m_eqGainInLinearInit, m_analysisWindowSize);
    //Envelope detectors
   
    for (int i = 0; i < numberOfEnvelopeDetectors; i++)
    {
        envelopeDetectors[i].init(in_pAllocator);
        envelopeDetectors[i].setDetectMode(m_envelopeDetectorMode);
        envelopeDetectors[i].setAttackTime(m_envelopeDetectorAttackTimeInSecInit);
        envelopeDetectors[i].setReleaseTime(m_envelopeDetectorReleaseTimeInSecInit);
        envelopeDetectors[i].setLookaheadStatus(m_envelopeDetectorLookAhead);
    }
}

void Vocoder::prepareToPlay(int samplingRate, AK::IAkPluginMemAlloc* in_pAllocator)
{
	m_samplingRate = samplingRate;

    m_analysisFIFO.clear(0.0f);
    m_synthesisFIFO.clear(0.0f);
    m_oscillatorFIFO.clear(0.0f);

    m_oscillator.setSampleRate(samplingRate);
    m_oscillator.resetPhase();

	m_pitchTracker.setAllocator(in_pAllocator);
    m_pitchTracker.prepareToPlay(samplingRate, m_analysisWindowSize);

    for (int i = 0; i < numberOfEnvelopeDetectors; i++)
    {
        envelopeDetectors[i].setSampleRate(static_cast<float>(samplingRate) / static_cast<float>(m_analysisHopSize));
    }

	fft.initialise(1024);
}

void Vocoder::processBlock(vector<float>& buffer, vector<float>& externalCarrier)
{
	//m_audioAnalysis.testFFT(buffer.data(), buffer.size());

	size_t numberOfSamples = buffer.size();
    for (int i = 0; i < numberOfSamples; i++)
    {
        m_analysisFIFO.write(buffer[i]);

        if (m_carrier == Carrier::OscillatorPitchTracking || m_carrier == Carrier::OscillatorFrequencySetByUI)
        {
            float pitch = m_pitchInterpolator.getNextValue();
            m_oscillator.setFrequency(pitch);

            m_oscillatorFIFO.write(m_oscillator.getNextSample());
        }
        else if (m_carrier == Carrier::OscillatorFrequencySetByUI)
        {
            m_oscillatorFIFO.write(m_oscillator.getNextSample());
        }
        else if (m_carrier == Carrier::Noise)
        {
            m_oscillatorFIFO.write(noiseOscillator.getNextSample());
        }
        else if (m_carrier == Carrier::External)
        {
            m_oscillatorFIFO.write(externalCarrier[i]);
        }
    }
	int count = 0;
    while (m_analysisFIFO.getNumberOfAvailableSamples() >= m_analysisWindowSize)
    {
        m_analysisFIFO.read(m_internalBuffer, m_analysisWindowSize, m_analysisHopSize);

        m_oscillatorFIFO.read(m_oscillatorBuffer, m_analysisWindowSize, m_analysisHopSize);

        //Analysis stage
        applyWindow(m_internalBuffer, m_analysisWindow, m_analysisWindowSize);

		fft.calculateFFT(m_internalBuffer);
		kiss_fft_cpx* modulatorSpectrum = (kiss_fft_cpx*) m_internalBuffer;

        updateCarrierFrequency(m_internalBuffer, modulatorSpectrum);
		extractSpectralEnvelope(modulatorSpectrum, m_fftSize, m_modulatorFftMagnitude, m_fftNormalizationCoefficient);
		
        //Processing stage
		fft.calculateFFT(m_oscillatorBuffer);
		kiss_fft_cpx* carrierSpectrum = (kiss_fft_cpx*) m_oscillatorBuffer;

        applyModulatorSpectralEnvelopeToCarrier(carrierSpectrum, m_modulatorFftMagnitude, m_nyquistBinIndex);
		applyEqGainsOnCarrierSpectrum(carrierSpectrum, m_fftSize, m_eqGain);


	
        //Synthesis stage
		fft.calculateIFFT(m_oscillatorBuffer);

		applyWindow(m_oscillatorBuffer, m_synthesisWindow, m_synthesisWindowSize);

        m_synthesisFIFO.add(m_oscillatorBuffer, m_synthesisWindowSize, m_synthesisHopSize);
		count++;
    }

    if (m_synthesisFIFO.getNumberOfAvailableSamples() >= numberOfSamples)
    {
        for (int i = 0; i < numberOfSamples; i++)
        {
			buffer[i] = m_synthesisFIFO.read();
        }
    }
    else
    {
        buffer.clear();
    }

    if (m_pitchTrackerAlgorithmHasChanged)
    {
        m_pitchTrackerAlgorithmHasChanged = false;
        m_pitchTracker.setAlgorithm(m_pitchTrackerAlgorithm);
    }
	
}

void Vocoder::processBlock(vector<float> &buffer)
{
    processBlock(buffer, buffer /* unused */);
}

void Vocoder::applyWindow(float* buffer, float* window, int bufferSize)
{
    for (int i = 0; i < bufferSize; i++)
    {
        buffer[i] *= window[i];
    }
}

bool Vocoder::sumOfOverlappingWindowsIsConstantTest()
{
    const int sizeSumOverlappingWindow = m_analysisWindowSize * 10;
    double sumOverlappingWindow[sizeSumOverlappingWindow];

    int numberOfWindows = sizeSumOverlappingWindow / m_analysisHopSize - m_analysisWindowSize / m_analysisHopSize + 1;

    for (int i = 0; i < sizeSumOverlappingWindow; i++)
    {
        sumOverlappingWindow[i] = 0;
    }

    for (int i = 0; i < numberOfWindows; i++)
    {
        int offset = i * m_analysisHopSize;

        for (int j = 0; j < m_analysisWindowSize; j++)
        {
            sumOverlappingWindow[offset + j] += m_synthesisWindow[j] * m_analysisWindow[j];
        }
    }

    return true;
}

void Vocoder::calculateEqualTemperamentNoteFrequencies(vector<float>& noteFrequencies)
{
    const float C0frequency = 16.3516f;

    const int numberOfNotes = 88;

    const float semitoneCoefficient = std::powf(2.0f, 1.0f / 12.0f);

    noteFrequencies.push_back(C0frequency);

    for (int i = 1; i < numberOfNotes; i++)
    {
        noteFrequencies.push_back(noteFrequencies[i - 1] * semitoneCoefficient);
    }
}

float Vocoder::findNearestValueInVector(float value, vector<float> vector)
{
    int nearestIndex = 0;

    float smallestDifference = std::fabs(vector[0] - value);

    for (int i = 1; i < vector.size(); i++)
    {
        float currentDifference = std::fabs(vector[i] - value);

        if (currentDifference < smallestDifference)
        {
            smallestDifference = currentDifference;
            nearestIndex = i;
        }
    }

    float nearestValue = vector[nearestIndex];

    return nearestValue;
}

void Vocoder::extractSpectralEnvelope(kiss_fft_cpx* inputFft, int fftSize, float* outputMagnitude, float normalizationCoefficient)
{
    for (int i = 0; i < fftSize; i++)
    {
        m_modulatorFftMagnitude[i] = normalizationCoefficient * std::sqrt(inputFft[i].r*inputFft[i].r + inputFft[i].i*inputFft[i].i);
    }
}

void Vocoder::extractSpectralEnvelope(float* fftBuffer, int fftSize, float* outputMagnitude, float normalizationCoefficient)
{
    for (int i = 0; i < fftSize; i++)
    {
        m_modulatorFftMagnitude[i] = normalizationCoefficient * std::sqrt(fftBuffer[(i*2)]* fftBuffer[(i * 2)] + fftBuffer[(i * 2)+1]* fftBuffer[(i * 2) + 1]);
    }
}

void Vocoder::applyEqGainsOnCarrierSpectrum(float* fftBuffer, int fftSize, float* gainBuffer)
{
	for (int i = 0; i < fftSize; i++)
	{
		fftBuffer[(i*2)] *= gainBuffer[i];
		fftBuffer[(i*2)+1] *= gainBuffer[i];
	}
}

void Vocoder::applyEqGainsOnCarrierSpectrum(kiss_fft_cpx* inputFft, int fftSize, float* gainBuffer)
{
    for (int i = 0; i < fftSize; i++)
    {
        inputFft[i].r *= gainBuffer[i];
        inputFft[i].i *= gainBuffer[i];
    }
}

void Vocoder::setCarrier(Carrier carrier)
{
    m_carrier = carrier;
}

void Vocoder::setCarrierFrequency(float carrierFrequency)
{
    m_carrierFrequency = carrierFrequency;
}

void Vocoder::setCarrierWaveform(WaveShapes waveform)
{
    m_oscillator.setWaveShape(waveform);
}

void Vocoder::setGainBin(float gain, unsigned int bandNumber, unsigned int numberOfBands)
{
    const int minimumSizeBand = 2;
    const int maxIndex = m_fftSize / 2; //Nyquist bin index [0, 1, ..., fftSize/2-1, NyquistBinIndex, fftSize/2+1, ..., fftSize-1]

    const float beta = static_cast<float>(minimumSizeBand - 1);
    const float alpha = (1.0 / static_cast<float>(numberOfBands - 1)) * std::logf(static_cast<float>(maxIndex) - 1.0 - beta);

    // bands are calculated using: f(x) = exp(alpha*x) + beta
    // f(0) = 2; (smallest bin must be 2 in order to affect other frequencies than DC)
    // f(numberOfBands - 1) = m_fftSize / 2; 
    float y1 = std::expf(alpha*static_cast<float>(bandNumber)) + beta;
    float y0 = (bandNumber == 0) ? 0.0f : std::expf(alpha*static_cast<float>(bandNumber - 1)) + beta;

    int sizeOfBand = std::ceilf(y1 - y0);

    for (int i = 0; i < sizeOfBand; i++)
    {
		// gain = 1.0f;	//testing only 

		int index = y0 + i;
        m_eqGain[index] = gain;

        int hermitianSymmetryIndex = m_fftSize - 1 - index;
        m_eqGain[hermitianSymmetryIndex] = gain;
    }
}

void Vocoder::setCarrierFrequencyMax(float carrierFrequencyMax)
{
    m_oscillatorFrequencyMax = carrierFrequencyMax;
}

void Vocoder::setCarrierFrequencyMin(float carrierFrequencyMin)
{
    m_oscillatorFrequencyMin = carrierFrequencyMin;
}

void Vocoder::setEnvelopAttackTime(float attackTimeInSeconds)
{
    for (int i = 0; i < numberOfEnvelopeDetectors; i++)
    {
        envelopeDetectors[i].setAttackTime(attackTimeInSeconds);
    }
}

void Vocoder::setEnvelopReleaseTime(float releaseTimeInSeconds)
{
    for (int i = 0; i < numberOfEnvelopeDetectors; i++)
    {
        envelopeDetectors[i].setReleaseTime(releaseTimeInSeconds);
    }
}

//Source: https://www.dsprelated.com/freebooks/sasp/Quadratic_Interpolation_Spectral_Peaks.html
float Vocoder::findMaxLocationUsingquadraticInterpolation(float yMinusOne, float yZero, float yPlusOne)
{
    float a = yMinusOne;
    float b = yZero;
    float g = yPlusOne;

    float denominator = a - 2.0f*b + g;

    float peakLocationInBins;

    if (denominator == 0.0f)
    {
        peakLocationInBins = 0.0f;
    }
    else
    {
        peakLocationInBins = 0.5f * (a - g) / denominator;
    }

    return peakLocationInBins;
}

void Vocoder::setPitchTrackerAlgorithm(PitchTracker::Algorithm algorithm)
{
	m_pitchTrackerAlgorithm = (PitchTracker::Algorithm)3;// algorithm;

    m_pitchTrackerAlgorithmHasChanged = false;
}

void Vocoder::applyModulatorSpectralEnvelopeToCarrier(float* fftBuffer, float* modulatorSpectrum, int halfFftSize)
{
	float envelope;
	const int numberOfSamples = 1;
	envelopeDetectors[0].getEnvelope(&modulatorSpectrum[0], &envelope, numberOfSamples);

	fftBuffer[0] *= envelope;
	fftBuffer[1] *= envelope;

	for (int i = 1; i < halfFftSize; i++)
	{
		envelopeDetectors[i].getEnvelope(&modulatorSpectrum[i], &envelope, numberOfSamples);

		fftBuffer[(i*2)] *= envelope;
		fftBuffer[(i*2)+1] *= envelope;

		fftBuffer[m_fftSize - (i*2)] *= envelope;
		fftBuffer[m_fftSize - (i*2)+1] *= envelope;
	}
	envelopeDetectors[numberOfEnvelopeDetectors - 1].getEnvelope(&modulatorSpectrum[halfFftSize], &envelope, numberOfSamples);

	fftBuffer[(halfFftSize*2)] *= envelope;
	fftBuffer[(halfFftSize*2)+1] *= envelope;

}

void Vocoder::applyModulatorSpectralEnvelopeToCarrier(kiss_fft_cpx* carrierSpectrum, float* modulatorSpectrum, int halfFftSize)
{
    float envelope;
    const int numberOfSamples = 1;
    envelopeDetectors[0].getEnvelope(&modulatorSpectrum[0], &envelope, numberOfSamples);

    carrierSpectrum[0].r *= envelope;
    carrierSpectrum[0].i *= envelope;

    for (int i = 1; i < halfFftSize; i++)
    {
        envelopeDetectors[i].getEnvelope(&modulatorSpectrum[i], &envelope, numberOfSamples);

        carrierSpectrum[i].r *= envelope;
        carrierSpectrum[i].i *= envelope;

		carrierSpectrum[m_fftSize - i].r = carrierSpectrum[i].r;
		carrierSpectrum[m_fftSize - i].i = carrierSpectrum[i].i;
    }

    envelopeDetectors[numberOfEnvelopeDetectors - 1].getEnvelope(&modulatorSpectrum[halfFftSize], &envelope, numberOfSamples);

    carrierSpectrum[halfFftSize].r *= envelope;
    carrierSpectrum[halfFftSize].i *= envelope;
}

void Vocoder::updateCarrierFrequency(float* fftBuffer)
{
	if (m_carrier == Carrier::OscillatorPitchTracking)
	{
		if (m_pitchTracker.getAlgorithm() == PitchTracker::Algorithm::SpectrumBased)
		{
			m_carrierFrequency = m_pitchTracker.processBlockAndGetPitch(fftBuffer);
		}
		else
		{
			// m_pitchTrackerBuffer.copyFrom(0, 0, buffer, m_pitchTrackerBufferSize);
			FloatVectorOperations::copy(m_pitchTrackerBuffer.data(), fftBuffer, m_pitchTrackerBufferSize);

			m_carrierFrequency = m_pitchTracker.processBlockAndGetPitch(m_pitchTrackerBuffer);
		}

		if (m_carrierFrequency < m_oscillatorFrequencyMin)
		{
			m_carrierFrequency = m_oscillatorFrequencyMin;
		}
		else if (m_carrierFrequency > m_oscillatorFrequencyMax)
		{
			m_carrierFrequency = m_oscillatorFrequencyMax;
		}

		//modulatorFrequency = findNearestValueInVector(modulatorFrequency, m_equalTemperamentNoteFrequencies);

		m_pitchInterpolator.setValue(m_carrierFrequency);
	}
	else if (m_carrier == Carrier::OscillatorFrequencySetByUI)
	{
		m_pitchInterpolator.setValue(m_carrierFrequency);
	}
}
void Vocoder::updateCarrierFrequency(float* buffer, kiss_fft_cpx* bufferFft)
{
    if (m_carrier == Carrier::OscillatorPitchTracking)
    {
        if (m_pitchTracker.getAlgorithm() == PitchTracker::Algorithm::SpectrumBased)
        {
            m_carrierFrequency = m_pitchTracker.processBlockAndGetPitch(bufferFft);
        }
        else
        {
           // m_pitchTrackerBuffer.copyFrom(0, 0, buffer, m_pitchTrackerBufferSize);
			FloatVectorOperations::copy(m_pitchTrackerBuffer.data(), buffer, m_pitchTrackerBufferSize);

            m_carrierFrequency = m_pitchTracker.processBlockAndGetPitch(m_pitchTrackerBuffer);
        }

        if (m_carrierFrequency < m_oscillatorFrequencyMin)
        {
            m_carrierFrequency = m_oscillatorFrequencyMin;
        }
        else if (m_carrierFrequency > m_oscillatorFrequencyMax)
        {
            m_carrierFrequency = m_oscillatorFrequencyMax;
        }

        //modulatorFrequency = findNearestValueInVector(modulatorFrequency, m_equalTemperamentNoteFrequencies);

        m_pitchInterpolator.setValue(m_carrierFrequency);
    }
    else if (m_carrier == Carrier::OscillatorFrequencySetByUI)
    {
        m_pitchInterpolator.setValue(m_carrierFrequency);
    }
}
