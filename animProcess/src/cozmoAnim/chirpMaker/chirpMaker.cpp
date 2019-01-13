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
void ChirpMaker::StartChattering( std::chrono::milliseconds delayUntilStart_ms, bool isHost )
{
  std::lock_guard<std::mutex> lk { _mutex };
  
  // stop anything else
  for( auto& analyzerPair : _analyzers ) {
    analyzerPair.second->Reset();
  }
  
  std::vector<Chirp> chirps;
  
  uint64_t nextStartTime = Sequencer::GetCurrTime() + (uint32_t)delayUntilStart_ms.count();
  
  const float G_Hz = 391.995f;
  const float C_Hz = 523.251f;
  
  // host does two notes up, then client two notes down
  if( isHost ) {
    Chirp chirp1;
    chirp1.startTime_ms = nextStartTime;
    chirp1.duration_ms = 500;
    chirp1.pitch0_Hz = chirp1.pitch1_Hz = G_Hz;
    chirp1.volume = 1.0;
    chirps.push_back(std::move(chirp1));
    
    Chirp chirp2;
    chirp2.startTime_ms = nextStartTime + 500;
    chirp2.duration_ms = 500;
    chirp2.pitch0_Hz = chirp2.pitch1_Hz = C_Hz;
    chirp2.volume = 1.0;
    chirps.push_back(std::move(chirp2));
  } else {
    Chirp chirp1;
    chirp1.startTime_ms = nextStartTime + 1000 + 500;
    chirp1.duration_ms = 500;
    chirp1.pitch0_Hz = chirp1.pitch1_Hz = C_Hz;
    chirp1.volume = 1.0;
    chirps.push_back(std::move(chirp1));
    
    Chirp chirp2;
    chirp2.startTime_ms = nextStartTime + 1000 + 500 + 500;
    chirp2.duration_ms = 500;
    chirp2.pitch0_Hz = chirp2.pitch1_Hz = G_Hz;
    chirp2.volume = 1.0;
    chirps.push_back(std::move(chirp2));
  }
  
  nextStartTime += 1000 + 500 + 1000 + 500;
  
  // host slides up and down, then client down and up
  if( isHost ) {
    Chirp chirp1;
    chirp1.startTime_ms = nextStartTime;
    chirp1.duration_ms = 500;
    chirp1.pitch0_Hz = G_Hz;
    chirp1.pitch1_Hz = C_Hz;
    chirp1.volume = 1.0;
    chirps.push_back(std::move(chirp1));
    
    Chirp chirp2;
    chirp2.startTime_ms = nextStartTime + 500;
    chirp2.duration_ms = 500;
    chirp2.pitch0_Hz = C_Hz;
    chirp2.pitch1_Hz = G_Hz;
    chirp2.volume = 1.0;
    chirps.push_back(std::move(chirp2));
  } else {
    Chirp chirp1;
    chirp1.startTime_ms = nextStartTime + 1000 + 500;
    chirp1.duration_ms = 500;
    chirp1.pitch0_Hz = C_Hz;
    chirp1.pitch1_Hz = G_Hz;
    chirp1.volume = 1.0;
    chirps.push_back(std::move(chirp1));
    
    Chirp chirp2;
    chirp2.startTime_ms = nextStartTime + 1000 + 500 + 500;
    chirp2.duration_ms = 500;
    chirp2.pitch0_Hz = G_Hz;
    chirp2.pitch1_Hz = C_Hz;
    chirp2.volume = 1.0;
    chirps.push_back(std::move(chirp2));
  }
  
  nextStartTime += 1000 + 500 + 1000 + 500;
  
  // host does a higher triplet, client a lower triplet
  if( isHost ) {
    Chirp chirp1;
    chirp1.startTime_ms = nextStartTime;
    chirp1.duration_ms = 90;
    chirp1.pitch0_Hz = chirp1.pitch1_Hz = C_Hz;
    chirp1.volume = 1.0;
    chirps.push_back(std::move(chirp1));
    
    Chirp chirp2;
    chirp2.startTime_ms = nextStartTime + 100;
    chirp2.duration_ms = 90;
    chirp2.pitch0_Hz = chirp2.pitch1_Hz = C_Hz;
    chirp2.volume = 1.0;
    chirps.push_back(std::move(chirp2));
    
    Chirp chirp3;
    chirp3.startTime_ms = nextStartTime + 200;
    chirp3.duration_ms = 90;
    chirp3.pitch0_Hz = chirp3.pitch1_Hz = C_Hz;
    chirp3.volume = 1.0;
    chirps.push_back(std::move(chirp3));
    
  } else {
    
    Chirp chirp1;
    chirp1.startTime_ms = nextStartTime + 500;
    chirp1.duration_ms = 90;
    chirp1.pitch0_Hz = chirp1.pitch1_Hz = G_Hz;
    chirp1.volume = 1.0;
    chirps.push_back(std::move(chirp1));
    
    Chirp chirp2;
    chirp2.startTime_ms = nextStartTime + 500 + 100;
    chirp2.duration_ms = 90;
    chirp2.pitch0_Hz = chirp2.pitch1_Hz = G_Hz;
    chirp2.volume = 1.0;
    chirps.push_back(std::move(chirp2));
    
    Chirp chirp3;
    chirp3.startTime_ms = nextStartTime + 500 + 200;
    chirp3.duration_ms = 90;
    chirp3.pitch0_Hz = chirp3.pitch1_Hz = G_Hz;
    chirp3.volume = 1.0;
    chirps.push_back(std::move(chirp3));
  }
  
  
  
  _context->GetSequencer()->AddChirps( chirps );
  
}
  
} // namespace Vector
} // namespace Anki
