/*
==============================================================================

NoiseOscillator.cpp
Created: 09 February 2017 12:36:11pm
Author:  Mathieu Carre

==============================================================================
*/

#include "NoiseOscillator.h"

NoiseOscillator::NoiseOscillator() :
    m_distribution(0.0, 1.0)
{

}

float NoiseOscillator::getNextSample()
{
    float value = m_distribution(m_generator);

    return value;
}

void NoiseOscillator::setRange(float min, float max)
{
    m_distribution = std::uniform_real_distribution<float>(min, max);
}
