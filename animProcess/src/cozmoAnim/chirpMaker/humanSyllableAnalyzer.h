/**
 * File: IAnalyzer.h
 *
 */

#ifndef ANIMPROCESS_COZMO_HUMANSYLLABLEANALYZER_H
#define ANIMPROCESS_COZMO_HUMANSYLLABLEANALYZER_H
#pragma once

#include "cozmoAnim/chirpMaker/IAnalyzer.h"

namespace Anki {
  namespace Util {
    template <typename T>
    class RingBuffContiguousRead;
  }
namespace Vector {
  
class SyllableDetector;
  class AnimContext;
  
class HumanSyllableAnalyzer : public IAnalyzer
{
public:
  HumanSyllableAnalyzer( Sequencer* seq, const AnimContext* context );
  
  void AddSamples( const int16_t* samples, unsigned int numSamples );
  
  void RunDetector();

private:
  
  const AnimContext* _context;
  
  std::unique_ptr<SyllableDetector> _syllableDetector;
  std::unique_ptr<Util::RingBuffContiguousRead<int16_t>> _buff;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_HUMANSYLLABLEANALYZER_H
