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
#include <util/helpers/noncopyable.h>
#include <util/signals/simpleSignal.hpp>
#include "clad/audio/audioMessageTypes.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioStateTypes.h"
#include "clad/audio/audioSwitchTypes.h"
#include "clad/audio/audioParameterTypes.h"
#include <vector>

namespace Anki {
namespace Cozmo {
namespace Audio {
  
class AudioEngineMessageHandler;
class MessageAudioClient;
// Callback Structs
struct AudioCallbackDuration;
struct AudioCallbackMarker;
struct AudioCallbackComplete;
struct AudioCallbackError;
  
class AudioEngineClient : Util::noncopyable
{
public:
  
  using CallbackIdType = uint16_t;
  
  void SetMessageHandler( AudioEngineMessageHandler* messageHandler );
  
  CallbackIdType PostEvent( EventType event,
                            uint16_t gameObjectId,
                            AudioCallbackFlag callbackFlag = AudioCallbackFlag::EventNone );

  void PostGameState( GameStateGroupType gameStateGroup, GameStateType gameState );
  
  void PostSwitchState( SwitchStateGroupType switchStateGroup, SwitchStateType switchState, uint16_t gameObjectId );
  
  void PostParameter( ParameterType parameter,
                      float parameterValue,
                      uint16_t gameObjectId,
                      int32_t timeInMilliSeconds,
                      CurveType curve ) const;
  
protected:
  
  AudioEngineMessageHandler* _messageHandler = nullptr;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  static constexpr CallbackIdType kInvalidCallbackId = 0;
  CallbackIdType _previousCallbackId = kInvalidCallbackId;
  
  void HandleEvents( const AnkiEvent<MessageAudioClient>& event );
  
  virtual void HandleCallbackEvent( const AudioCallbackDuration& callbackMsg );
  virtual void HandleCallbackEvent( const AudioCallbackMarker& callbackMsg );
  virtual void HandleCallbackEvent( const AudioCallbackComplete& callbackMsg );
  virtual void HandleCallbackEvent( const AudioCallbackError& callbackMsg );
  
  CallbackIdType GetNewCallbackId();
  
};

} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_AudioEngineClient_H__ */
