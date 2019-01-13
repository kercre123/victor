/**
 * File: sequencer.h
 *
 */

#ifndef ANIMPROCESS_COZMO_SEQUENCER_H
#define ANIMPROCESS_COZMO_SEQUENCER_H
#pragma once

#include "clad/types/chirpTypes.h"

#include <mutex>
#include <thread>
#include <vector>
#include <list>
#include <set>


namespace Anki {
namespace AudioEngine {
class AudioEngineController;
}
namespace Util {
  class IConsoleFunction;
}

namespace Vector {
  
class AnimContext;
struct Chirp;

class Sequencer
{
public:
  
  Sequencer();
  ~Sequencer();
  
  void Init( const AnimContext* context );
  
  void Update();
  
  void AddChirp( const Chirp& chirp );
  void AddChirps( const std::vector<Chirp>& chirps );
  
  bool HasChirps() const;
  
  void Test_Triplet( const float pitch_Hz, const uint32_t duration_ms );
  void Test_Pitch( const float pitch0_Hz, const float pitch1_Hz, const uint32_t duration_ms );
  void Test_ShaveHaircut( const uint32_t quarterNode_ms, uint32_t delay_ms = 1000 );
  
  static uint64_t GetCurrTime();
private:
  using Clock = std::chrono::steady_clock;
  
  void MainLoop();
  
  void AddChirpInternal( const Chirp& chirp );
  
  float PitchToRelativeCents( const float pitch_Hz );
  
  
  std::chrono::time_point<Clock> ConvertToTimePoint( uint64_t time_ms ) const;
  
  AudioEngine::AudioEngineController* _audioController = nullptr;
  
  mutable std::mutex _mutex;
  std::thread _thread;
  std::condition_variable _condVar;
  std::atomic<bool> _running;
  
  std::chrono::time_point<Clock> _waitUntil;
  
  struct ChirpInfo {
    ChirpInfo( const Chirp& chirp ) : chirp{chirp} {}
    inline uint64_t GetStartTime() const { return chirp.startTime_ms; }
    inline uint64_t GetEndTime() const { return chirp.startTime_ms + chirp.duration_ms; }
    inline bool ConstantPitch() const { return chirp.pitch1_Hz == chirp.pitch0_Hz; }
    inline float InterpPitch(uint64_t t) const { return ConstantPitch() ? chirp.pitch1_Hz : (t - GetStartTime())
                                                  * ((chirp.pitch1_Hz - chirp.pitch0_Hz) / (GetEndTime() - GetStartTime()))
                                                  + chirp.pitch0_Hz; }
    
    bool playing = false;
    Chirp chirp;
  };
  struct ChirpInfoCompare {
    bool operator()( const ChirpInfo& lhs, const ChirpInfo& rhs ) const {
      if( lhs.GetStartTime() == rhs.GetStartTime() ) {
        if( lhs.playing ) {
          return true;
        } else if( rhs.playing ) {
          return false;
        } else {
          return (lhs.chirp.duration_ms < rhs.chirp.duration_ms);
        }
      } else {
        return (lhs.GetStartTime() < rhs.GetStartTime());
      }
    }
  };
  std::set<ChirpInfo, ChirpInfoCompare> _chirps;
  
  int _octave = std::numeric_limits<int>::max();
  
  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
