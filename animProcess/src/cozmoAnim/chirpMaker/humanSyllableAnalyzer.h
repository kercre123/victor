/**
 * File: IAnalyzer.h
 *
 */

#ifndef ANIMPROCESS_COZMO_HUMANSYLLABLEANALYZER_H
#define ANIMPROCESS_COZMO_HUMANSYLLABLEANALYZER_H
#pragma once

#include "cozmoAnim/chirpMaker/IAnalyzer.h"

namespace Anki {
namespace Vector {
  
class HumanSyllableAnalyzer : public IAnalyzer
{
public:
  explicit HumanSyllableAnalyzer( Sequencer* seq );
  
  void AddSamples( const int16_t* samples, unsigned int numSamples );
  
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_HUMANSYLLABLEANALYZER_H
