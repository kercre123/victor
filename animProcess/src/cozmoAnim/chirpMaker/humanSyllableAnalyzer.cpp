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
#include "util/helpers/ankiDefines.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/backpackLights/animBackpackLightComponent.h"

#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>

namespace Anki {
namespace Vector {
  
namespace {
  constexpr unsigned int kBuffSize = 3*16000; // 1 sec
  
  CONSOLE_VAR_RANGED(float, kTimeScaleFactor, "Chirps", 1.2f, 0.5f, 2.0f);
  CONSOLE_VAR(bool, kUseTestFile, "Chirps", false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HumanSyllableAnalyzer::HumanSyllableAnalyzer( Sequencer* seq, const AnimContext* context )
  : IAnalyzer( seq )
  , _context{ context }
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
  } else {
    _context->GetBackpackLightComponent()->SetAlexaStreaming( true );
  }
  
  
  if( _buff->Capacity() <= _buff->Size() + numSamples ) {
    _context->GetBackpackLightComponent()->SetAlexaStreaming( false );
    PRINT_NAMED_WARNING("Chirps", "Running human detector: %zu %zu, %d", _buff->Capacity(), _buff->Size(), numSamples);
    // this should probably be a job on another thread so we don't block mic input
    RunDetector();
  }
  _buff->AddData( samples, numSamples );
  
}
  
  
  void SavePCM( const short* buff, size_t size =0 )
  {
    static int pcmfd = -1;
    if( pcmfd < 0 ) {
      static int cnt = 0;
#if defined(ANKI_PLATFORM_VICOS)
      const auto path = std::string{"/data/data/com.anki.victor/cache/buff"} + std::to_string(cnt) + ".pcm";
#else
      const auto path = std::string{"/Users/rossanderson/victor/simulator/controllers/webotsCtrlAnim/cache/buff"} + std::to_string(cnt) + ".pcm";
#endif
      ++cnt;
      pcmfd = open( path.c_str(), O_CREAT|O_RDWR|O_TRUNC, 0644 );
    }
    
    if( size > 0 && (buff != nullptr) ) {
      (void) write( pcmfd, buff, size * sizeof(short) );
    } else if( pcmfd >= 0 ) {
      close( pcmfd );
      pcmfd = -1;
    }
  }
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HumanSyllableAnalyzer::RunDetector()
{
  
  unsigned int buffSize = static_cast<unsigned int>( _buff->GetContiguousSize() );
  const auto* buff = _buff->ReadData( buffSize );
  
  std::vector<short> input;
  if( kUseTestFile ) {
#if defined(ANKI_PLATFORM_VICOS)
    const char* filename = "/data/data/com.anki.victor/persistent/fromRobot.pcm";// thisIsMeTalking.pcm";// "heyDude.pcm";//tellMeSomething.pcm"; "bathroom.pcm";//
#else
    const char* filename = "/Users/rossanderson/victor/simulator/controllers/webotsCtrlAnim/persistent/fromRobot.pcm";
#endif
    
    std::ifstream fin(filename, std::ios::in|std::ios::binary);
    
    short iSample = 0;
    while( fin.read((char *)&iSample,sizeof(short)) )
    {
      input.push_back( iSample );
    }
    buff = input.data();
    buffSize = static_cast<unsigned int>(input.size());
  }
  
//  SavePCM(buff, buffSize);
//  SavePCM(nullptr);
  
  const auto syllables = _syllableDetector->Run( buff, buffSize );
  
  _buff->AdvanceCursor( buffSize );
  
  if( syllables.empty() ) {
    PRINT_NAMED_INFO("WHATNOW", "no syllables");
  }
  
  if( std::any_of(syllables.begin(), syllables.end(), [](const auto& c){ return c.avgPower >= -90; }) ) {
    
    using namespace std::chrono;
    uint64_t startTime_ms = duration_cast<milliseconds>(steady_clock::now().time_since_epoch() + milliseconds{1000}).count();
    std::vector<Chirp> chirps;
    for( const auto& syllable : syllables ) {
      PRINT_NAMED_WARNING("WHATNOW","Syllable info: start=%f, end=%f, freq=%f, vol=%f",
                          syllable.startTime_s, syllable.endTime_s, syllable.avgFreq, syllable.avgPower );
      
      if( syllable.endIdx <= syllable.startIdx + 1 ) {
        continue;
      }
      
      Chirp chirp;
      chirp.startTime_ms = startTime_ms + kTimeScaleFactor*syllable.startTime_s*1000;
      chirp.duration_ms = (syllable.endTime_s - syllable.startTime_s)*1000;
      
      chirp.duration_ms *= kTimeScaleFactor;
      
      //float freq = std::min(syllable.peakFreq, syllable.avgFreq);
      float freq0 = syllable.firstFreq; //syllable.avgFreq;
      float freq1 = syllable.lastFreq; //syllable.avgFreq;
      freq0 *= 15625.0f/16000; // rescale based on actual syscon freq and 16k
      freq1 *= 15625.0f/16000; // rescale based on actual syscon freq and 16k
  //    if( freq0 > 400 ) {
  //      freq0 /= 2;
  //      freq1 /= 2;
  //    } else if( freq < 120 ) {
  //      freq *= 2;
  //    }
      chirp.pitch0_Hz = freq0;
      chirp.pitch1_Hz = freq1;
      chirp.volume = 1.0;
      chirps.push_back( std::move(chirp) );
    }
    GetSequencer()->AddChirps(chirps);
  }
  
}
  
} // namespace Vector
} // namespace Anki
