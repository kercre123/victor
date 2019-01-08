/**
 * File: sequencer.cpp
 *
 */

#include "cozmoAnim/chirpMaker/humanSyllableAnalyzer.h"
#include "cozmoAnim/chirpMaker/sequencer.h"
#include "cozmoAnim/chirpMaker/syllableDetector.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/container/ringBuffContiguousRead.h"

#include <chrono>

namespace Anki {
namespace Vector {
  
namespace {
  constexpr unsigned int kBuffSize = 3*16000; // 3 sec
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HumanSyllableAnalyzer::HumanSyllableAnalyzer( Sequencer* seq )
  : IAnalyzer( seq )
  , _buff{ std::make_unique<Util::RingBuffContiguousRead<int16_t>>(kBuffSize, kBuffSize) }
{
  
  const unsigned int sampleRate_Hz = 16000;
  const unsigned int windowLen = 512;
  const unsigned int overlap = 128;
  const unsigned int fftLen = 2*windowLen;
  // DFT length of 2*512 = 1024, zero padded
  _syllableDetector = std::make_unique<SyllableDetector>(sampleRate_Hz, windowLen, overlap, fftLen);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HumanSyllableAnalyzer::AddSamples( const int16_t* samples, unsigned int numSamples )
{
  if( GetSequencer()->HasChirps() ) {
    // wait until everything has been played
    return;
  }
  
  
  if( _buff->Capacity() <= _buff->Size() + numSamples ) {
    PRINT_NAMED_WARNING("Chirps", "Running human detector: %zu %zu, %d", _buff->Capacity(), _buff->Size(), numSamples);
    // this should probably be a job on another thread so we don't block mic input
    RunDetector();
  }
  _buff->AddData( samples, numSamples );
  
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HumanSyllableAnalyzer::RunDetector()
{
  
  const unsigned int buffSize = static_cast<unsigned int>( _buff->GetContiguousSize() );
  const auto* buff = _buff->ReadData( buffSize );
  const auto syllables = _syllableDetector->Run( buff, buffSize );
  
  _buff->AdvanceCursor( buffSize );
  
  if( syllables.empty() ) {
    PRINT_NAMED_INFO("WHATNOW", "no syllables");
  }
  
  using namespace std::chrono;
  uint64_t startTime_ms = duration_cast<milliseconds>(steady_clock::now().time_since_epoch() + milliseconds{1000}).count();
  std::vector<Chirp> chirps;
  for( const auto& syllable : syllables ) {
    PRINT_NAMED_WARNING("WHATNOW","Syllable info: start=%f, end=%f, freq=%f, vol=%f",
                        syllable.startTime_s, syllable.endTime_s, syllable.avgFreq, syllable.avgPower );
    if( syllable.avgPower < -80.0f ) {
      continue;
    }
    
    
    Chirp chirp;
    chirp.startTime_ms = startTime_ms + syllable.startTime_s*1000;
    chirp.duration_ms = (syllable.endTime_s - syllable.startTime_s)*1000;
    
    if( chirp.duration_ms != chirp.duration_ms || chirp.duration_ms < 30 ) {
      continue;
    }
    
    chirp.pitch0_Hz = syllable.avgFreq;
    chirp.pitch1_Hz = syllable.avgFreq;
    chirp.volume = 1.0;
    chirps.push_back( std::move(chirp) );
  }
  GetSequencer()->AddChirps(chirps);
  
}
  
} // namespace Vector
} // namespace Anki
