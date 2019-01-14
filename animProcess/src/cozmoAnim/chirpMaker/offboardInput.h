/**
 * File: IAnalyzer.h
 *
 */

#ifndef ANIMPROCESS_COZMO_OFFBOARDINPUT_H
#define ANIMPROCESS_COZMO_OFFBOARDINPUT_H
#pragma once

#include "cozmoAnim/chirpMaker/IAnalyzer.h"

#include <list>

namespace Anki {
namespace Util {
  class IConsoleFunction;
}
namespace Vector {
  
class SyllableDetector;
struct Chirp;
  
class OffboardInput : public IAnalyzer
{
public:
  explicit OffboardInput( Sequencer* seq );
  
  void AddSamples( const int16_t* samples, unsigned int numSamples );

private:
  void Parse(const std::string& data);
  
  std::vector<Chirp> _chirps;
  
  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;
  mutable std::mutex _mutex;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_OFFBOARDINPUT_H
