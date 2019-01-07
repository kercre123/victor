/**
 * File: sequencer.cpp
 *
 */

#include "cozmoAnim/chirpMaker/humanSyllableAnalyzer.h"
#include "cozmoAnim/chirpMaker/sequencer.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#include <chrono>

namespace Anki {
namespace Vector {
  
namespace {
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HumanSyllableAnalyzer::HumanSyllableAnalyzer( Sequencer* seq )
  : IAnalyzer( seq )
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HumanSyllableAnalyzer::AddSamples( const int16_t* samples, unsigned int numSamples )
{
  // todo: analyze and send commands to GetSequencer()
}
  
} // namespace Vector
} // namespace Anki
