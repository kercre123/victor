#ifndef WAVETABLEOSCILLATOR_H_INCLUDED
#define WAVETABLEOSCILLATOR_H_INCLUDED

#include "Wavetables.h"

using namespace std;

enum WaveShapes {
    Sinusoidal = 0,
    Triangle = 1, 
    Sawtooth = 2,
    ReverseSawtooth = 3,
    Square = 4,
    Tangent = 8,
    Sinc=14,
    BandlimitedImpulseTrain=20,
    BandlimitedSawtooth=22,
    BandlimitedSquare=24,
    NumberOfWaveshapes
};

class WavetableOscillator {
    
public:
    
    WavetableOscillator();
        
    enum Interpolation {
        NONE,
        LINEAR,
        CUBIC
    };
    
    void setSampleRate (double newSampleRate);
    void setFrequency(float newFreq);
    void setWaveShape(WaveShapes newShape);
    void setInterpolationToUse(Interpolation newSetting);
    
    float getFrequency() { return frequencyInHz; }
    
    void resetPhase() { phase = 0.0f; }
    void resetPhase(float offsetInRadians);
    
    
    float getNextSample();
    
    void processBuffer(vector<float>& buffer);
    
    
private:
    
    float updateWithoutInterpolation();
    float updateWithLinearInterpolation();
    float updateWithCubicInterpolation();
    
    float phase         {  0.0f };
    float increment     {  1.0f };
    float frequencyInHz {   440 };
    float sampleRate    { 44100 };

    size_t tableSize      {  32 };
    
    WaveShapes shape { Sinusoidal };
    Interpolation interpolationSetting { CUBIC };
};



#endif 
