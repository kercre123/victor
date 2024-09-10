/*
 * File: audioMuxClient.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is a base class that provides communication between its self and an AudioMuxInput by means of that
 *              is defined in the sub-class. It provides core audio functionality by using Audio CLAD messages as the
 *              transport interface.
 *
 * Copyright: Anki, Inc. 2015
 *
 */


#ifndef __AnkiAudio_AudioMuxClient_H__
#define __AnkiAudio_AudioMuxClient_H__

#include "audioEngine/audioExport.h"
#include "audioEngine/multiplexer/audioCallback.h"
#include "clad/audio/audioBusTypes.h"
#include "clad/audio/audioCallbackMessage.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/audio/audioMessageTypes.h"
#include "clad/audio/audioParameterTypes.h"
#include "clad/audio/audioStateTypes.h"
#include "clad/audio/audioSwitchTypes.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include <functional>
#include <vector>
#include <unordered_map>


namespace Anki {
namespace AudioEngine {
namespace Multiplexer {
  
class AUDIOENGINE_EXPORT AudioMuxClient : private Anki::Util::noncopyable
{
public:
  
  using CallbackIdType = uint16_t;
  using CallbackFunc = std::function< void ( const AudioCallback& callback ) >;
  
  AudioMuxClient();
  virtual ~AudioMuxClient();
  
  // Perform event
  // Provide a callback lambda to get all event callbacks; Duration, Marker, Complete & Error.
  virtual CallbackIdType PostEvent( AudioMetaData::GameEvent::GenericEvent event,
                                    AudioMetaData::GameObjectType gameObject = AudioMetaData::GameObjectType::Invalid,
                                    CallbackFunc&& callback = nullptr ) = 0;
  
  virtual void StopAllEvents( AudioMetaData::GameObjectType gameObject = AudioMetaData::GameObjectType::Invalid );

  virtual void PostGameState( AudioMetaData::GameState::StateGroupType gameStateGroup,
                              AudioMetaData::GameState::GenericState gameState );
  
  virtual  void PostSwitchState( AudioMetaData::SwitchState::SwitchGroupType switchGroup,
                                 AudioMetaData::SwitchState::GenericSwitch switchState,
                                 AudioMetaData::GameObjectType gameObject = AudioMetaData::GameObjectType::Invalid );
  
  virtual void PostParameter( AudioMetaData::GameParameter::ParameterType parameter,
                              float parameterValue,
                              AudioMetaData::GameObjectType gameObject = AudioMetaData::GameObjectType::Invalid,
                              int32_t timeInMilliSeconds = 0,
                              CurveType curve = CurveType::Linear ) const;
  
  virtual void PostMusicState( AudioMetaData::GameState::GenericState musicState,
                               bool interrupt = false,
                               uint32_t minDuration_ms = 0 );


protected:
  
  static constexpr CallbackIdType kInvalidCallbackId = 0;
  CallbackIdType _previousCallbackId = kInvalidCallbackId;
  
  using CallbackMap = std::unordered_map< CallbackIdType, CallbackFunc >;
  CallbackMap _callbackMap;
  
  // Handle specific types of callbacks
  void HandleCallbackEvent( const AudioCallbackDuration& callbackMsg );
  void HandleCallbackEvent( const AudioCallbackMarker& callbackMsg );
  void HandleCallbackEvent( const AudioCallbackComplete& callbackMsg );
  void HandleCallbackEvent( const AudioCallbackError& callbackMsg );
  
  // Store callback function for callback
  CallbackIdType ManageCallback( CallbackFunc&& callbackFunc );
  
  // Call stored callback with Audio Callback Union type
  void PerformCallback( const AudioCallback& callbackData );
  
  // Get the next "unique" callback id
  CallbackIdType GetNewCallbackId();
  
};


} // Multiplexer
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_AudioMuxClient_H__ */
