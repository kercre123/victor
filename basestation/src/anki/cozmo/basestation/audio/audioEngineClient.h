/*
 * File: audioEngineClient.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is a subclass of AudioClient which provides communication between itself and an
 *              AudioEngineClientConnection by means of AudioEngineMessageHandler. It provides core audio functionality
 *              by broadcasting post messages and subscribing to callback messages.
 *
 * Copyright: Anki, Inc. 2015
 *
 */

#ifndef __Basestation_Audio_AudioEngineClient_H__
#define __Basestation_Audio_AudioEngineClient_H__

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "clad/audio/audioBusses.h"
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
namespace Cozmo {
namespace Audio {
  
class AudioEngineMessageHandler;
class MessageAudioClient;
struct AudioCallback;
  
class AudioEngineClient : Util::noncopyable
{
public:
  
  using CallbackIdType = uint16_t;
  using CallbackFunc = std::function< void ( const AudioCallback& callback ) >;
  
  void SetMessageHandler( AudioEngineMessageHandler* messageHandler );
  
  // Perform event
  // Provide a callback lambda to get all event callbacks; Duration, Marker, Complete & Error.
  CallbackIdType PostEvent( const GameEvent::GenericEvent event,
                            const GameObjectType gameObject = GameObjectType::Default,
                            const CallbackFunc& callback = nullptr );
  
  void StopAllEvents( const GameObjectType gameObject = GameObjectType::Invalid );

  void PostGameState( const GameState::StateGroupType gameStateGroup,
                      const GameState::GenericState gameState );
  
  void PostSwitchState( const SwitchState::SwitchGroupType switchGroup,
                        const SwitchState::GenericSwitch switchState,
                        const GameObjectType gameObject = GameObjectType::Default );
  
  void PostParameter( const GameParameter::ParameterType parameter,
                      const float parameterValue,
                      const GameObjectType gameObject = GameObjectType::Default,
                      const int32_t timeInMilliSeconds = 0,
                      const CurveType curve = CurveType::Linear ) const;
  
  void PostMusicState( const GameState::GenericState musicState,
                       const bool interrupt = false,
                       const uint32_t minDuration_ms = 0 );

protected:
  
  AudioEngineMessageHandler* _messageHandler = nullptr;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  static constexpr CallbackIdType kInvalidCallbackId = 0;
  CallbackIdType _previousCallbackId = kInvalidCallbackId;
  
  using CallbackMap = std::unordered_map< CallbackIdType, CallbackFunc >;
  CallbackMap _callbackMap;
  
  void HandleCallbackEvent( const AudioCallback& callbackMsg );
  
  CallbackIdType GetNewCallbackId();
  
};

} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_AudioEngineClient_H__ */
