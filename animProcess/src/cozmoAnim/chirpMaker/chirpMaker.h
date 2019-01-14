/**
 * File: sequencer.h
 *
 */

#ifndef ANIMPROCESS_COZMO_CHIRPMAKER_H
#define ANIMPROCESS_COZMO_CHIRPMAKER_H
#pragma once

#include <cstdint>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace Anki {
namespace Vector {
  
class IAnalyzer;
class AnimContext;
  
class ChirpMaker
{
public:
  
  ChirpMaker();
  ~ChirpMaker();
  
  void Init( const AnimContext* context );
  
  void AddSamples( const int16_t* samples, unsigned int numSamples );
  
  // demo method to play some tune
  void StartChattering( std::chrono::milliseconds delayUntilStart_ms, bool isHost );
  
private:
  
  const AnimContext* _context = nullptr;
  
  enum class ChirpMode {
    None=0,
    HumanSyllables,
    OffboardInput,
  };
  
  std::unordered_map<ChirpMode, std::unique_ptr<IAnalyzer>> _analyzers;
  
  mutable std::mutex _mutex;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_CHIRPMAKER_H
