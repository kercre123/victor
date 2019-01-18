/**
 * File: sequencer.cpp
 *
 */

#include "cozmoAnim/chirpMaker/offboardInput.h"
#include "cozmoAnim/chirpMaker/sequencer.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"

#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>

namespace Anki {
namespace Vector {
  
namespace {
  
  // poll this to see if there are still chirps being played. dont actually set it
  CONSOLE_VAR(bool, kStillProcessing, "Chirps.Offboard", false);
  
  CONSOLE_VAR(bool, kUseTestFile, "Chirps", false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffboardInput::OffboardInput( Sequencer* seq )
  : IAnalyzer( seq )
{
  auto func = [this](ConsoleFunctionContextRef context) {
    std::string data = ConsoleArg_GetOptional_String(context, "data", "");
    Parse(data);
  };
  _consoleFuncs.emplace_front("ProvideInput", std::move(func), "Chirps.Offboard", "const char* data" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OffboardInput::AddSamples( const int16_t* samples, unsigned int numSamples )
{
  kStillProcessing = GetSequencer()->HasChirps();
  if( kStillProcessing ) {
    // wait until everything has been played
    return;
  }
  
  std::lock_guard<std::mutex> lk{_mutex};
  
  if( _chirps.empty() ) {
    return;
  }
  
  uint64_t currTime_ms = Sequencer::GetCurrTime();
  for( auto& chirp : _chirps ) {
    chirp.startTime_ms += currTime_ms;
    PRINT_NAMED_WARNING("Chirps", "offboard chirp start=%lld, duration=%d, pitch0=%f, pitch1=%f, vol=%f", chirp.startTime_ms, chirp.duration_ms, chirp.pitch0_Hz, chirp.pitch1_Hz, chirp.volume);
  }
  // advance start time of all chirps to _now_
  GetSequencer()->AddChirps( _chirps );
  _chirps.clear();
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OffboardInput::Parse( const std::string& data )
{
  std::lock_guard<std::mutex> lk{_mutex};
  
  const auto chirpStrings = Util::StringSplit(data, '-');
  PRINT_NAMED_WARNING("WHATNOW", "Got %zu strings", chirpStrings.size());
  for( const auto& chirpString : chirpStrings ) {
    const auto params = Util::StringSplit(chirpString, '_');
    if( params.size() == 5 ) {
      Chirp chirp;
      try {
        chirp.startTime_ms = std::stoll(params[0]);
        chirp.duration_ms = std::stoi(params[1]);
        chirp.pitch0_Hz = std::stof(params[2]);
        chirp.pitch1_Hz = std::stof(params[3]);
        chirp.volume = std::stof(params[4]);
        _chirps.push_back(std::move(chirp));
      } catch( std::exception _) {
        PRINT_NAMED_WARNING("Chirps", "Could not parse values in chirp string '%s'", chirpString.c_str());
      }
    } else {
      PRINT_NAMED_WARNING("Chirps", "Wrong number of params in chirp string '%s'", chirpString.c_str());
    }
  }
  
}
  
} // namespace Vector
} // namespace Anki
