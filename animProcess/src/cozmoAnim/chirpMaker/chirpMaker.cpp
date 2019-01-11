/**
 * File: sequencer.cpp
 *
 */

#include "cozmoAnim/chirpMaker/chirpMaker.h"
#include "clad/types/chirpTypes.h"
#include "cozmoAnim/chirpMaker/sequencer.h"
#include "cozmoAnim/chirpMaker/IAnalyzer.h"
#include "cozmoAnim/chirpMaker/humanSyllableAnalyzer.h"
#include "cozmoAnim/animContext.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#include <chrono>

namespace Anki {
namespace Vector {
  
namespace {
  CONSOLE_VAR_ENUM( unsigned int, kChirpMode, "Chirps", 0, "None,HumanSyllables" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ChirpMaker::ChirpMaker()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ChirpMaker::~ChirpMaker()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ChirpMaker::Init(const AnimContext* context)
{
  _context = context;
  auto* sequencer = _context->GetSequencer();
  
  // create analyzers for each mode
  _analyzers[ChirpMode::HumanSyllables].reset( new HumanSyllableAnalyzer{sequencer, _context} );
  
  // todo: something for analyzing another vector's chirps
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ChirpMaker::AddSamples( const int16_t* samples, unsigned int numSamples )
{
  std::lock_guard<std::mutex> lk { _mutex };
  
  auto it = _analyzers.find( static_cast<ChirpMode>(kChirpMode) );
  if( (it != _analyzers.end()) && (it->second != nullptr) ) {
    it->second->AddSamples( samples, numSamples );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ChirpMaker::StartChattering( std::chrono::milliseconds delayUntilStart_ms )
{
  std::lock_guard<std::mutex> lk { _mutex };
  
  // stop anything else
  for( auto& analyzerPair : _analyzers ) {
    analyzerPair.second->Reset();
  }
  
  // play shave and a haircut
  _context->GetSequencer()->Test_ShaveHaircut( 500, (uint32_t)delayUntilStart_ms.count() );
  
}
  
} // namespace Vector
} // namespace Anki
