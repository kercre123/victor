/*
 * File: audioEngineClient.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is a sub-class of AudioClient which provides communication between its self and an
 *              AudioEngineClientConnection by means of AudioEngineMessageHandler. It provide core audio functionality
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
  using CallbackFunc = std::function< void ( AudioCallback callback ) >;
  
  void SetMessageHandler( AudioEngineMessageHandler* messageHandler );
  
  // Perform event
  // Provide a callback lambda to get all event callbacks; Duration, Marker, Complete & Error.
  CallbackIdType PostEvent( GameEvent::GenericEvent event,
                            GameObjectType gameObject = GameObjectType::Default,
                            CallbackFunc callback = nullptr );
  
  void StopAllEvents( GameObjectType gameObject = GameObjectType::Invalid );

  void PostGameState( GameState::StateGroupType gameStateGroup,
                      GameState::GenericState gameState );
  
  void PostSwitchState( SwitchState::SwitchGroupType switchGroup,
                        SwitchState::GenericSwitch switchState,
                        GameObjectType gameObject = GameObjectType::Default );
  
  void PostParameter( GameParameter::ParameterType parameter,
                      float parameterValue,
                      GameObjectType gameObject = GameObjectType::Default,
                      int32_t timeInMilliSeconds = 0,
                      CurveType curve = CurveType::Linear ) const;
  
  void PostMusicState( GameState::GenericState musicState,
                       bool interrupt,
                       uint32_t minDuration_ms );

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
