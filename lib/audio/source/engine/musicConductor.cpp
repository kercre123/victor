/*
 * File: musicConductor.cpp
 *
 * Author: Jordan Rivas
 * Created: 06/14/16
 *
 * Description: This class tracks and manages the current Music state and transitions.
 *
 * Copyright: Anki, Inc. 2016
 *
 */

#include "audioEngine/audioEngineController.h"
#include "audioEngine/musicConductor.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "util/time/universalTime.h"


namespace Anki {
namespace AudioEngine {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MusicConductor::MusicConductor( AudioEngineController& audioEngine, const MusicConductorConfig& config )
: _audioEngine( audioEngine )
, _musicGameObject( config.musicGameObject )
, _musicStateId( config.musicGroupId )
, _startEventId( config.startEventId )
, _stopEventId( config.stopEventId )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MusicConductor::SetMusicState( AudioStateId stateId,
                                    bool interrupt,
                                    uint32_t minDuratoin_ms )
{
  DEV_ASSERT(stateId != kInvalidAudioEventId, "MusicConductor.SetMusicState.stateId.IsInvalidAudioEventId");
  
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
  
  if ( Anki::Util::IsFltGEZero( _currentStateExpirationTimeStamp_ms ) ) {
    // Check if minimum duration has passed
    if ( Anki::Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() < _currentStateExpirationTimeStamp_ms ) {
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
  _audioEngine.PostAudioEvent( _stopEventId, _musicGameObject );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MusicConductor::UpdateMusicState()
{
  DEV_ASSERT(_pendingStateId != kInvalidAudioEventId,
             "MusicConductor.UpdateMusicState._pendingStateId.IsInvalidAudioEventId");
  
  _currentStateId = _pendingStateId;
  if ( _pendingStateMinDuration_ms != kZeroDuration ) {
    _currentStateExpirationTimeStamp_ms = Anki::Util::Time::UniversalTime::GetCurrentTimeInMilliseconds() + _pendingStateMinDuration_ms;
  }
  else {
    _currentStateExpirationTimeStamp_ms = 0.0;
  }
  _pendingStateId = kInvalidAudioEventId;
  _pendingStateMinDuration_ms = kZeroDuration;
  
  if ( !_isPlaying ) {
    // Post Music Start event
    const AudioPlayingId playId = _audioEngine.PostAudioEvent( _startEventId, _musicGameObject );
    if ( playId != kInvalidAudioPlayingId ) {
      _isPlaying = true;
    }
    else {
      PRINT_NAMED_WARNING( "MusicConductor.SetState", "Start Music event FAILED!" );
    }
  }
  
  PRINT_CH_INFO(AudioEngineController::kLogChannelName,
                "MusicConductor.UpdateMusicState",
                "StateId: %ul", _currentStateId);
  
  const bool success = _audioEngine.SetState( _musicStateId, _currentStateId );
  if ( !success ) {
    PRINT_NAMED_ERROR( "MusicConductor.UpdateMusicState", "Failed to set Music State %d", _currentStateId );
  }
}

} // AudioEngine
} // Anki
