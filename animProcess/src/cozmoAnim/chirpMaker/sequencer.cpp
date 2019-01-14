/**
 * File: sequencer.cpp
 *
 */

#include "cozmoAnim/chirpMaker/sequencer.h"

#include "cozmoAnim/animProcessMessages.h" // must come before clad includes........

#include "cozmoAnim/audio/cozmoAudioController.h"

#include "audioEngine/audioTypes.h"
#include "audioEngine/audioTypeTranslator.h"
#include "audioUtil/audioDataTypes.h"
#include "coretech/common/engine/utils/timer.h"
#include "cozmoAnim/animContext.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include <chrono>

namespace Anki {
namespace Vector {
  
namespace {
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;
  
  const uint32_t kTestDelay_ms = 1000;
  const TimePoint kInvalidTime = Clock::now();
  
  uint32_t kPitchTick_ms = 10;
  
  CONSOLE_VAR_RANGED(float, kSourcePitch_Hz, "Chirps", 150.0f, 0.0f, 5000.0f);
  CONSOLE_VAR_RANGED(float, kMinCentsSlider_Hz, "Chirps", -500.0f, -10000.0f, 0.0f);
  CONSOLE_VAR_RANGED(float, kMaxCentsSlider_Hz, "Chirps", 500.0f, 0.0f, 10000.0f);
  
  const bool kRTPCIsCents = false; // true if cents, false if pich
  CONSOLE_VAR_RANGED(float, kMinPitchSlider_Hz, "Chirps", 440, 0.F, 5000.0F);
  CONSOLE_VAR_RANGED(float, kMaxPitchSlider_Hz, "Chirps", 1760, 0.0f, 5000.0f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sequencer::Sequencer()
 : _waitUntil{ kInvalidTime }
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sequencer::~Sequencer()
{
  if( _thread.joinable() ) {
    _running = false;
    _condVar.notify_one();
    _thread.join();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sequencer::Init(const AnimContext* context)
{
  if( context != nullptr ) {
    // could happen in unit tests
    _audioController = context->GetAudioController();
  }
  
  auto shaveAndHaircut = [&](ConsoleFunctionContextRef context ) {
    const uint32_t quarterNote_ms = ConsoleArg_Get_UInt32( context, "quarterNote_ms");
    Test_ShaveHaircut( quarterNote_ms );
  };
  auto triplet = [&](ConsoleFunctionContextRef context ) {
    const uint32_t duration_ms = ConsoleArg_Get_UInt32( context, "duration_ms");
    const float pitch_Hz = ConsoleArg_Get_Float( context, "pitchHz");
    Test_Triplet( pitch_Hz, duration_ms );
  };
  auto changingPitch = [&](ConsoleFunctionContextRef context ) {
    const uint32_t duration_ms = ConsoleArg_Get_UInt32( context, "duration_ms");
    const float pitch0_Hz = ConsoleArg_Get_Float( context, "minHz");
    const float pitch1_Hz = ConsoleArg_Get_Float( context, "maxHz");
    Test_Pitch( pitch0_Hz, pitch1_Hz, duration_ms );
  };
  _consoleFuncs.emplace_front( "ShaveAndAHairCutTwoBits", std::move(shaveAndHaircut), "Chirps", "uint32_t quarterNote_ms" );
  _consoleFuncs.emplace_front( "Triplet", std::move(triplet), "Chirps", "uint32_t duration_ms, float pitchHz" );
  _consoleFuncs.emplace_front( "ChangingPitch", std::move(changingPitch), "Chirps", "uint32_t duration_ms, float minHz, float maxHz" );
  
  _running = true;
  _thread = std::thread{ &Sequencer::MainLoop, this };
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sequencer::AddChirp( const Chirp& chirp )
{
  {
    std::lock_guard<std::mutex> lk{_mutex};
    AddChirpInternal( chirp );
  }
  _condVar.notify_one();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sequencer::AddChirps( const std::vector<Chirp>& chirps )
{
  _octave = std::numeric_limits<int>::max();
  {
    std::lock_guard<std::mutex> lk{_mutex};
    for( const auto& chirp : chirps ) {
      AddChirpInternal( chirp );
    }
  }
  _condVar.notify_one();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sequencer::AddChirpInternal( const Chirp& chirp )
{
  // make sure all chirps are disjoint before adding
  Chirp copy = chirp;
  for( const auto& existingChirp : _chirps ) {
    const auto existingEnd = existingChirp.GetEndTime();
    const auto newEnd = copy.startTime_ms + copy.duration_ms;
    if( (existingChirp.chirp.startTime_ms <= copy.startTime_ms)
         && (existingEnd >= newEnd) )
    {
      // don't insert if there's one already playing. this could be improved
      PRINT_NAMED_WARNING("Chirps", "New chirp fully contained by existing chirp");
      return;
    } else if( (existingEnd > copy.startTime_ms) && (copy.startTime_ms >= existingChirp.GetStartTime() ) ) {
      copy.startTime_ms = existingEnd;
      copy.duration_ms = static_cast<uint32_t>(newEnd - copy.startTime_ms);
      PRINT_NAMED_WARNING("Chirps", "New chirp overlapped with existing chirp; delaying start time of new chirp");
    } else if( (existingChirp.GetStartTime() < newEnd) && (newEnd <= existingChirp.GetEndTime()) ) {
      copy.duration_ms = static_cast<uint32_t>(existingChirp.GetStartTime() - copy.startTime_ms);
      PRINT_NAMED_WARNING("Chirps", "New chirp overlapped with existing chirp; stopping new chirp sooner");
    }
  }
  
  PRINT_NAMED_INFO( "Chirps", "Added new chirp, start=%lld ms, duration=%d ms, pitch0=%f Hz, pitch1=%f Hz, vol=%f",
                    copy.startTime_ms, copy.duration_ms, copy.pitch0_Hz, copy.pitch1_Hz, copy.volume);
  _chirps.emplace( std::move(copy) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sequencer::Update()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sequencer::MainLoop()
{
  using namespace AudioEngine;
  const auto gameObject = ToAudioGameObject( AudioMetaData::GameObjectType::Procedural );
  using GE = AudioMetaData::GameEvent::GenericEvent;
  using GP = AudioMetaData::GameParameter::ParameterType;
  
  auto sendStart = [&](uint32_t duration_ms) {
    const float durationParam = Util::Clamp((float)duration_ms, 0.0f, 1000.0f)/1000.0f;
    if( _audioController != nullptr ) {
      _audioController->SetParameter( ToAudioParameterId( GP::Victor_Robot_Chirps_Duration ),
                                      durationParam,
                                      gameObject );
      const auto eventId = ToAudioEventId( GE::Play__Robot_Vic_Sfx__Chirps );
      _audioController->PostAudioEvent( eventId, gameObject );
    } else {
      const auto t = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - kInvalidTime).count();
      PRINT_NAMED_INFO( "Chirps", "[t=%lld]: Playing with duration %f", t, durationParam );
    }
  };
  auto sendStop = [&]() {
    if( _audioController != nullptr ) {
      const auto eventId = ToAudioEventId( GE::Stop__Robot_Vic_Sfx__Chirps );
      _audioController->PostAudioEvent( eventId, gameObject );
    } else {
      const auto t = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - kInvalidTime).count();
      PRINT_NAMED_INFO("Chirps", "[t=%lld]: Stopping", t);
    }
  };
  auto sendPitchVolume = [&](float pitch, float volume) {
    float param;
    if( kRTPCIsCents ) {
      const float cents = PitchToRelativeCents( pitch );
      param = (cents - kMinCentsSlider_Hz) / (kMaxCentsSlider_Hz - kMinCentsSlider_Hz);// + kMinCentsSlider_Hz;
      param = Util::Clamp(param, 0.0f, 1.0f);
      PRINT_NAMED_WARNING("WHATNOW","cents=%f, param=%f", cents, param);
    } else {
      const float transposed = PitchToOctavedPitch(pitch);
      param = (transposed - kMinPitchSlider_Hz) / (kMaxPitchSlider_Hz - kMinPitchSlider_Hz);
      param = Util::Clamp(param, 0.0f, 1.0f);
      PRINT_NAMED_WARNING("WHATNOW","pitch=%f, transposed=%f, param=%f", pitch, transposed, param);
    }
    
    if( _audioController != nullptr ) {
      _audioController->SetParameter( ToAudioParameterId( GP::Victor_Robot_Chirps_Pitch ),
                                      param,
                                      gameObject );
      _audioController->SetParameter( ToAudioParameterId( GP::Victor_Robot_Chirps_Amplitude ),
                                      Util::Clamp(volume, 0.0f, 1.0f),
                                      gameObject );
    } else {
      const auto t = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - kInvalidTime).count();
      PRINT_NAMED_INFO("Chirps", "[t=%lld]: Setting rtpc=%f, vol=%f", t, param, volume);
    }
  };
  
  
  while( _running ) {
    
    std::unique_lock<std::mutex> lk{ _mutex };
    if( _waitUntil == kInvalidTime ) {
      _condVar.wait( lk );
    } else {
      _condVar.wait_until( lk, _waitUntil );
    }
    // we were woken up. either some chirp was added, or a playing chirp should be ending
    
    bool fluctuatingPitch = false;
    
    size_t cntPlaying = std::count_if( _chirps.begin(), _chirps.end(), [](const auto& c) { return c.playing; });
    ANKI_VERIFY( cntPlaying <= 1, "", "");
    
    bool stillPlaying = false;
    
    const auto currTime_ms = GetCurrTime();
    
    uint64_t nextWakeTime = std::numeric_limits<uint64_t>::max();
    
    // find things that need stopping
    for( auto it = _chirps.begin(); it != _chirps.end(); ) {
      auto& chirpInfo = *it;
      if( chirpInfo.playing ) {
        if( (currTime_ms >= chirpInfo.GetEndTime()) ) {
          // send stop event
          sendStop();
          it = _chirps.erase( it );
        } else if( !chirpInfo.ConstantPitch() ) {
          // still playing, and pitch is fluctuating
          stillPlaying = true;
          const float pitch = chirpInfo.InterpPitch( currTime_ms );
          const float volume = chirpInfo.chirp.volume;
          fluctuatingPitch = true;
          sendPitchVolume(pitch, volume);
          nextWakeTime = std::min(nextWakeTime, chirpInfo.GetEndTime());
          ++it;
        } else {
          // still playing at constant pitch, nothing to do here
          stillPlaying = true;
          ++it;
        }
      } else if( currTime_ms >= chirpInfo.GetEndTime() ) {
        // somehow missed this one
        PRINT_NAMED_WARNING("Chirps", "Accidentally skipped one");
        it = _chirps.erase( it );
      } else {
        ++it;
      }
    }
    
    // find things that need playing
    for( auto it = _chirps.begin(); it != _chirps.end();  ) {
      auto& chirpInfo = *it;
      if( !chirpInfo.playing ) {
        if( (currTime_ms >= chirpInfo.GetStartTime()) && (currTime_ms <= chirpInfo.GetEndTime()) ) {
          ANKI_VERIFY( !stillPlaying,"","" );
          
          // copy and erase in order to modify playing flag. it would be cleaner to have a map<ChirpInfo,bool>, but
          // that way we cant also sort on the playing flag
          auto chirpCopy = chirpInfo;
          it = _chirps.erase( it );
          chirpCopy.playing = true;
          
          nextWakeTime = std::min( nextWakeTime, chirpCopy.GetEndTime());
          
          fluctuatingPitch = !chirpCopy.ConstantPitch();
          
          const float pitch = chirpCopy.InterpPitch( currTime_ms );
          const float volume = chirpCopy.chirp.volume;
          
          sendPitchVolume(pitch, volume);
          sendStart( chirpInfo.chirp.duration_ms );
          
          _chirps.insert( chirpCopy );
        } else {
          nextWakeTime = std::min( nextWakeTime, it->GetStartTime());
          ++it;
        }
      } else {
        ++it;
      }
    }
    
    if( fluctuatingPitch ) {
      _waitUntil = Clock::now() + std::chrono::milliseconds{ kPitchTick_ms }; // ConvertToTimePoint(currTime_ms + kPitchTick_ms);
      PRINT_NAMED_INFO("Chirps", "sleeping for %lld",
                       std::chrono::duration_cast<std::chrono::milliseconds>(_waitUntil-Clock::now()).count());
    } else if( nextWakeTime != std::numeric_limits<uint64_t>::max() ) {
      _waitUntil = Clock::now() + std::chrono::milliseconds{ nextWakeTime - currTime_ms };// ConvertToTimePoint(nextWakeTime);
      PRINT_NAMED_INFO("Chirps", "sleeping for %lld = %lld",
                       nextWakeTime - currTime_ms, std::chrono::duration_cast<std::chrono::milliseconds>(_waitUntil-Clock::now()).count());
    } else {
      _waitUntil = kInvalidTime;
      PRINT_NAMED_INFO( "Chirps", "sleeping until chirps are added" );
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint64_t Sequencer::GetCurrTime()
{
  using namespace std::chrono;
  auto now = Clock::now();
  auto now_ms = time_point_cast<milliseconds>(now);
  auto value = now_ms.time_since_epoch();
  return value.count();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::chrono::time_point<Clock> Sequencer::ConvertToTimePoint( uint64_t time_ms ) const
{
  std::chrono::duration<uint64_t> duration{time_ms};
  std::chrono::time_point<Clock> dt( duration );
  return dt;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sequencer::Test_Triplet( const float pitch_Hz, const uint32_t duration_ms )
{
  _octave = std::numeric_limits<int>::max();
  
  const uint64_t startTime_ms = GetCurrTime() + kTestDelay_ms;
  Chirp chirp1 = {
    .startTime_ms = startTime_ms,
    .duration_ms = duration_ms,
    .volume = 1.0f,
  };
  chirp1.pitch0_Hz = pitch_Hz;
  chirp1.pitch1_Hz = chirp1.pitch0_Hz;
  
  Chirp chirp2 = chirp1;
  Chirp chirp3 = chirp1;
  chirp2.startTime_ms = chirp1.startTime_ms + chirp1.duration_ms;
  chirp3.startTime_ms = chirp2.startTime_ms + chirp2.duration_ms;
  AddChirp(chirp1);
  AddChirp(chirp2);
  AddChirp(chirp3);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sequencer::Test_Pitch( const float pitch0_Hz, const float pitch1_Hz, const uint32_t duration_ms )
{
  _octave = std::numeric_limits<int>::max();
  
  const uint64_t startTime_ms = GetCurrTime() + kTestDelay_ms;
  Chirp chirp = {
    .startTime_ms = startTime_ms,
    .duration_ms = duration_ms,
    .volume = 0.5f,
    .pitch0_Hz = pitch0_Hz,
    .pitch1_Hz = pitch1_Hz,
  };
  AddChirp( chirp );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sequencer::Test_ShaveHaircut( uint32_t quarterNode_ms, uint32_t delay_ms )
{
  _octave = std::numeric_limits<int>::max();
  
  std::vector<Chirp> chirps;
  
  const float G_Hz = 391.995f;
  const float D_Hz = 293.665f;
  const float E_Hz = 329.628f;
  const float Fsharp_Hz = 369.994f;
  
  uint64_t nextStartTime = GetCurrTime() + delay_ms;
  
  auto emplaceChirp = [&](const float pitch_Hz, uint32_t duration_ms) {
    Chirp chirp = {
      .startTime_ms = nextStartTime,
      .duration_ms = duration_ms,
      .volume = 1.0f,
      .pitch0_Hz = pitch_Hz,
      .pitch1_Hz = pitch_Hz,
    };
    nextStartTime += duration_ms;
    chirps.push_back( std::move(chirp) );
  };
  
  emplaceChirp( G_Hz, quarterNode_ms );
  emplaceChirp( D_Hz, quarterNode_ms/2 );
  emplaceChirp( D_Hz, quarterNode_ms/2 );
  emplaceChirp( E_Hz, quarterNode_ms );
  emplaceChirp( D_Hz, quarterNode_ms );
  nextStartTime += quarterNode_ms;
  emplaceChirp( Fsharp_Hz, quarterNode_ms );
  emplaceChirp( G_Hz, quarterNode_ms );
  
  AddChirps( chirps );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Sequencer::HasChirps() const
{
  std::lock_guard<std::mutex> lk{_mutex};
  return !_chirps.empty();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float Sequencer::PitchToRelativeCents( const float pitch_Hz )
{
  // find the octave that brings kSourcePitch_Hz closest to ~300 Hz. call the pitch at that octave pitchB_Hz.
  // Then find the deviation in cents from pitch_Hz to pitchB_Hz.
  float minDiff = std::numeric_limits<float>::max();
  if( _octave == std::numeric_limits<int>::max() ) {
    // can be optimized i know
    for( int i=0; i<10; ++i ) {
      const float f1 = std::pow(2.0, i) * pitch_Hz;
      float diff = fabs(f1 - kSourcePitch_Hz);
      if( diff < minDiff ) {
        minDiff = diff;
        _octave = i;
      }
      const float f2 = std::pow(2.0, -i) * pitch_Hz;
      //PRINT_NAMED_WARNING("WHATNOW", "trying octave=%d, pitch=%f", -i, f2);
      diff = fabs(f2 - kSourcePitch_Hz);
      if( diff < minDiff ) {
        minDiff = diff;
        _octave = -i;
      }
    }
  }
  
  const float pitchB_Hz = std::pow(2.0, _octave) * pitch_Hz;
  // now find deviation of pitchB from the source pitch
  const float cents = 1200 * log2( pitchB_Hz / kSourcePitch_Hz );
  PRINT_NAMED_WARNING("WHATNOW", "input=%f, octave=%d, closest=%f, cents=%f", pitch_Hz, _octave, pitchB_Hz, cents);
  return cents;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float Sequencer::PitchToOctavedPitch( const float pitch_Hz )
{
  float minDiff = std::numeric_limits<float>::max();
  if( _octave == std::numeric_limits<int>::max() ) {
    const float midPitch = (kMinPitchSlider_Hz + kMaxPitchSlider_Hz)/2;
    // can be optimized i know
    for( int i=0; i<10; ++i ) {
      const float f1 = std::pow(2.0, i) * pitch_Hz;
      float diff = fabs(f1 - midPitch);
      if( diff < minDiff ) {
        minDiff = diff;
        _octave = i;
      }
      const float f2 = std::pow(2.0, -i) * pitch_Hz;
      diff = fabs(f2 - midPitch);
      if( diff < minDiff ) {
        minDiff = diff;
        _octave = -i;
      }
    }
  }
  
  const float pitchB_Hz = std::pow(2.0, _octave) * pitch_Hz;
  return pitchB_Hz;
}
  
} // namespace Vector
} // namespace Anki
