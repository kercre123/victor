
#include "WavetableOscillator.h"
#include <math.h>
#ifndef ORBIS
#define M_PI 3.14159265359
#endif

WavetableOscillator::WavetableOscillator()
{
}

void WavetableOscillator::setSampleRate (double newSampleRate)
{
    if (newSampleRate != 0) {
        sampleRate = newSampleRate;

        increment = static_cast<float>(tableSize) * (frequencyInHz / sampleRate);
    }
}

void WavetableOscillator::setFrequency(float newFreq)
{
    frequencyInHz = newFreq;
    
    increment = static_cast<float>(tableSize) * (frequencyInHz / sampleRate);
}

void WavetableOscillator::setWaveShape(WaveShapes newShape)
{
    shape = newShape;
}

void WavetableOscillator::setInterpolationToUse(Interpolation newSetting)
{
    interpolationSetting = newSetting;
}

void WavetableOscillator::resetPhase(float offsetInRadians)
{
    while (offsetInRadians < 0) {
        offsetInRadians += 2 * M_PI;
    }
    while (offsetInRadians > (2 * M_PI)) {
        offsetInRadians -= 2 * M_PI;
    }
    
    phase = (offsetInRadians / (2 * M_PI)) * tableSize;
}

float WavetableOscillator::getNextSample()
{
    if (interpolationSetting == NONE) {
        return updateWithoutInterpolation();
    }
    else if (interpolationSetting == LINEAR){
        return updateWithLinearInterpolation();
    }
    else {
        return updateWithCubicInterpolation();
    }
}

void WavetableOscillator::processBuffer(vector<float>& buffer)
{
    if (interpolationSetting == NONE) {

        /*for (int chan = 0; chan < buffer.size(); ++chan) {
            
            float* chanPointer = buffer.getWritePointer(chan);
            
            for (int i = 0; i < buffer.getNumSamples(); ++i) {

                chanPointer[i] = updateWithoutInterpolation();
            }
        }*/
		for (int i = 0; i < buffer.size(); i++)
		{
			buffer[i] = updateWithoutInterpolation();
		}
    }
    else if (interpolationSetting == LINEAR){
        
        /*for (int chan = 0; chan < buffer.getNumChannels(); ++chan) {
            
            float* chanPointer = buffer.getWritePointer(chan);
            
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                
                chanPointer[i] = updateWithLinearInterpolation();
            }
        }*/

		for (int i = 0; i < buffer.size(); i++)
		{
			buffer[i] = updateWithLinearInterpolation();
		}
    }
    else {
        
       /* for (int chan = 0; chan < buffer.getNumChannels(); ++chan) {
            
            float* chanPointer = buffer.getWritePointer(chan);
            
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                
                chanPointer[i] = updateWithCubicInterpolation();
            }
        }*/

		for (int i = 0; i < buffer.size(); i++)
		{
			buffer[i] = updateWithCubicInterpolation();
		}
    }
}

float WavetableOscillator::updateWithoutInterpolation()
{
    int i = (int) phase;
    
    phase = fmod((phase + increment), tableSize);
    
    if (shape == Sinusoidal) {
        return Wavetables::sinTable[i];
    }
    else {
        return 0.0f;
    }
}
              
              
float WavetableOscillator::updateWithLinearInterpolation()
{
    int i = static_cast<int>(phase);
    float alpha = phase - static_cast<float>(i);
    
    phase = fmod((phase + increment), tableSize);
    float value = 0.0;
    
    switch (shape) {
    case Sinusoidal: 
        value = Wavetables::sinTable[i] + Wavetables::sinTableInterp0[i] * alpha; 
        break;
#ifdef TRIANGLE_WAVETABLE
    case Triangle:   
        value = Wavetables::triangleTable[i] + Wavetables::triangleTableInterp0[i] * alpha;
        break;
#endif
#ifdef SAWTOOTH_WAVETABLE
    case Sawtooth:   
        value = Wavetables::sawtoothTable[i] + Wavetables::sawtoothTableInterp0[i] * alpha;
        break;
#endif
#ifdef REVERSE_SAWTOOTH_WAVETABLE
    case ReverseSawtooth:   
        value = Wavetables::reverseSawtoothTable[i] + Wavetables::reverseSawtoothTableInterp0[i] * alpha;
        break;
#endif
#ifdef SQUARE_WAVETABLE
	case Square:   
        value = Wavetables::squareTable[i] + Wavetables::squareTableInterp0[i] * alpha;
        break;
#endif
#ifdef TANGENT_WAVETABLE
    case Tangent:   
        value = Wavetables::tangentTable[i] + Wavetables::tangentTableInterp0[i] * alpha; 
        break;
#endif
#ifdef SINC_WAVETABLE
    case Sinc:   
        value = Wavetables::sincTable[i] + Wavetables::sincTableInterp0[i] * alpha;
        break;
#endif
#ifdef BANDLIMITED_IMPULSE_TABLE
    case BandlimitedImpulseTrain:   
        value = Wavetables::bandlimitedImpulseTable[i] + Wavetables::bandlimitedImpulseTableInterp0[i] * alpha;
        break;
#endif
#ifdef BANDLIMITED_SAWTOOTH_WAVETABLE
    case BandlimitedSawtooth:   
        value = Wavetables::bandlimitedSawtoothTable[i] + Wavetables::bandlimitedSawtoothTableInterp0[i] * alpha;
        break;
#endif
#ifdef BANDLIMITED_SQUARE_WAVETABLE
    case BandlimitedSquare:   
        value = Wavetables::bandlimitedSquareTable[i] + Wavetables::bandlimitedSquareTableInterp0[i] * alpha;
        break;
#endif
    default:        
        value = Wavetables::sinTable[i] + Wavetables::sinTableInterp0[i] * alpha;
        break;
    }

    return value;
}
              
              
float WavetableOscillator::updateWithCubicInterpolation()
{
    int i = static_cast<int>(phase);
    float alpha = phase - static_cast<float>(i);
    
    tableSize = Wavetables::sinTable.size(); //all tables must be the same size
    phase = fmod((phase + increment), tableSize);
    
    /* //remember to wrap around!!!
     dtable1[i] = (3.f*(table[i]-table[i+1])-table[i-1]+table[i+2])/2.f
     dtable2[i] = 2.f*table[i+1]+table[i-1]-(5.f*table[i]+table[i+2])/2.f
     dtable3[i] = (table[i+1]-table[i-1])/2.f
     */

    float value;
    switch (shape) {
    case Sinusoidal:
        value = ((Wavetables::sinTableInterp1[i] * alpha + Wavetables::sinTableInterp2[i]) * alpha + Wavetables::sinTableInterp3[i]) * alpha + Wavetables::sinTable[i];
        break;
#ifdef TRIANGLE_WAVETABLE
    case Triangle:
        value =  ((Wavetables::triangleTableInterp1[i] * alpha + Wavetables::triangleTableInterp2[i]) * alpha + Wavetables::triangleTableInterp3[i]) * alpha + Wavetables::triangleTable[i];		
        break;
#endif
#ifdef SAWTOOTH_WAVETABLE
	case Sawtooth:
        value = ((Wavetables::sawtoothTableInterp1[i] * alpha + Wavetables::sawtoothTableInterp2[i]) * alpha + Wavetables::sawtoothTableInterp3[i]) * alpha + Wavetables::sawtoothTable[i];
        break;
#endif
#ifdef REVERSE_SAWTOOTH_WAVETABLE
    case ReverseSawtooth:
        value = ((Wavetables::reverseSawtoothTableInterp1[i] * alpha + Wavetables::reverseSawtoothTableInterp2[i]) * alpha + Wavetables::reverseSawtoothTableInterp3[i]) * alpha + Wavetables::reverseSawtoothTable[i];
        break;
#endif
#ifdef SQUARE_WAVETABLE
    case Square:
        value = ((Wavetables::squareTableInterp1[i] * alpha + Wavetables::squareTableInterp2[i]) * alpha + Wavetables::squareTableInterp3[i]) * alpha + Wavetables::squareTable[i];
        break;
#endif
#ifdef TANGENT_WAVETABLE
    case Tangent:
        value = ((Wavetables::tangentTableInterp1[i] * alpha + Wavetables::tangentTableInterp2[i]) * alpha + Wavetables::tangentTableInterp3[i]) * alpha + Wavetables::tangentTable[i];
        break;
#endif
#ifdef SINC_WAVETABLE
    case Sinc:
        value = ((Wavetables::sincTableInterp1[i] * alpha + Wavetables::sincTableInterp2[i]) * alpha + Wavetables::sincTableInterp3[i]) * alpha + Wavetables::sincTable[i];
        break;
#endif
#ifdef BANDLIMITED_IMPULSE_WAVETABLE
    case BandlimitedImpulseTrain:
        value = ((Wavetables::bandlimitedImpulseTableInterp1[i] * alpha + Wavetables::bandlimitedImpulseTableInterp2[i]) * alpha + Wavetables::bandlimitedImpulseTableInterp3[i]) * alpha + Wavetables::bandlimitedImpulseTable[i];
        break;
#endif
#ifdef BANDLIMITED_SAWTOOTH_WAVETABLE
    case BandlimitedSawtooth:
        value = ((Wavetables::bandlimitedSawtoothTableInterp1[i] * alpha + Wavetables::bandlimitedSawtoothTableInterp2[i]) * alpha + Wavetables::bandlimitedSawtoothTableInterp3[i]) * alpha + Wavetables::bandlimitedSawtoothTable[i];
        break;
#endif
#ifdef BANDLIMITED_SQUARE_WAVETABLE
    case BandlimitedSquare:
        value = ((Wavetables::bandlimitedSquareTableInterp1[i] * alpha + Wavetables::bandlimitedSquareTableInterp2[i]) * alpha + Wavetables::bandlimitedSquareTableInterp3[i]) * alpha + Wavetables::bandlimitedSquareTable[i];
        break;
#endif
    default:
        value = ((Wavetables::sinTableInterp1[i] * alpha + Wavetables::sinTableInterp2[i]) * alpha + Wavetables::sinTableInterp3[i]) * alpha + Wavetables::sinTable[i];
        break;
    }
    
    return value;
}