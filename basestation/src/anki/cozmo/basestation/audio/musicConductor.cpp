//
//  MusicConductor.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 6/14/16.
//
//

#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/musicConductor.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "util/time/universalTime.h"


namespace Anki {
namespace Cozmo {
namespace Audio {
  
using namespace AudioEngine;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MusicConductor::MusicConductor( AudioController& audioController,
                                AudioEngine::AudioGameObject musicGameObject,
                                AudioEngine::AudioStateGroupId musicGroupId,
                                AudioEngine::AudioEventId startEventId,
                                AudioEngine::AudioEventId stopEventId )
: _audioController( audioController )
, _musicGameObject( musicGameObject )
, _musicStateId( musicGroupId )
, _startEventId( startEventId )
, _stopEventId( stopEventId )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MusicConductor::SetMusicState( AudioEngine::AudioStateId stateId,
                                    bool interrupt,
                                    uint32_t minDuratoin_ms )
{
  ASSERT_NAMED( stateId != kInvalidAudioEventId, "MusicConductor.SetMusicState.stateId.IsInvalidAudioEventId" );
  
  std::lock_guard<std::mutex> lock( _lock );
  if ( interrupt ) {
    // Reset expiration date to change state next updateTick()
    _currentStateExpirationTimeStamp_ms = 0.0;
  }
  
  _pendingStateId = stateId;
  _pendingStateMinDuration_ms = minDuratoin_ms;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MusicConductor::UpdateTick()
{
  std::lock_guard<std::mutex> lock( _lock );
  // Check for pending music state
  if ( _pendingStateId == kInvalidAudioEventId ) {
    return;
  }
  
  if ( Util::IsFltGEZero( _currentStateExpirationTimeStamp_ms ) ) {
    // Check if minimum duration has passed
    if ( Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() < _currentStateExpirationTimeStamp_ms ) {
      return;
    }
  }
  
  UpdateMusicState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MusicConductor::StopMusic()
{
  std::lock_guard<std::mutex> lock( _lock );
  _isPlaying = false;
  _pendingStateId = kInvalidAudioEventId;
  _audioController.PostAudioEvent( _stopEventId, _musicGameObject );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MusicConductor::UpdateMusicState()
{
  ASSERT_NAMED( _pendingStateId != kInvalidAudioEventId,
                "MusicConductor.UpdateMusicState._pendingStateId.IsInvalidAudioEventId" );
  
  _currentStateId = _pendingStateId;
  if ( _pendingStateMinDuration_ms != kZeroDuration ) {
    _currentStateExpirationTimeStamp_ms = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() + _pendingStateMinDuration_ms;
  }
  else {
    _currentStateExpirationTimeStamp_ms = 0.0;
  }
  _pendingStateId = kInvalidAudioEventId;
  _pendingStateMinDuration_ms = kZeroDuration;
  
  if ( !_isPlaying ) {
    // Post Music Start event
    const AudioPlayingId playId = _audioController.PostAudioEvent( _startEventId, _musicGameObject );
    if ( playId != kInvalidAudioPlayingId ) {
      _isPlaying = true;
    }
    else {
      PRINT_NAMED_WARNING( "MusicConductor.SetState", "Start Music event FAILED!" );
    }
  }
  
  PRINT_CH_INFO(AudioController::kAudioLogChannelName,
                "MusicConductor.UpdateMusicState",
                "StateId: %ul", _currentStateId);
  
  const bool success = _audioController.SetState( _musicStateId, _currentStateId );
  if ( !success ) {
    PRINT_NAMED_ERROR( "MusicConductor.UpdateMusicState", "Failed to set Music State %d", _currentStateId );
  }
}

} // Audio
} // Cozmo
} // Anki
