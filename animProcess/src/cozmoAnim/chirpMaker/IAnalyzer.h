/**
 * File: IAnalyzer.h
 *
 */

#ifndef ANIMPROCESS_COZMO_IANALYZER_H
#define ANIMPROCESS_COZMO_IANALYZER_H
#pragma once

#include "cozmoAnim/chirpMaker/sequencer.h"

namespace Anki {
namespace Vector {
  
class IAnalyzer
{
public:
  explicit IAnalyzer( Sequencer* seq ) : _sequencer{seq} {}
  
  virtual ~IAnalyzer() { Reset(); }
  
  virtual void AddSamples( const int16_t* samples, unsigned int numSamples ) = 0;
  
  virtual void Reset() {}
  
protected:
  Sequencer* GetSequencer() const { return _sequencer; }
  
private:
  Sequencer* _sequencer;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
