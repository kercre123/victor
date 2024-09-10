#ifndef NOISEOSCILLATOR_H_INCLUDED
#define NOISEOSCILLATOR_H_INCLUDED

/*
==============================================================================

NoiseOscillator.h
Created: 09 February 2017 12:36:11pm
Author:  Mathieu Carre

==============================================================================
*/

#include <random>




//==============================================================================
/**
*/
class NoiseOscillator 
{
public:
    NoiseOscillator();

    float getNextSample();

    void setRange(float min, float max);

private:
    std::default_random_engine m_generator;
    std::uniform_real_distribution<float> m_distribution;
};

#endif
