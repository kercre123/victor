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

class AudioEngineClient : Util::noncopyable
{
public:
  
  AudioEngineClient( AudioEngineMessageHandler& messageHandler );
  
  void PostEvent( EventType event,
                  uint16_t gameObjectId,
                  AudioCallbackFlag callbackFlag = AudioCallbackFlag::EventNone ) const;

  void PostGameState( GameStateType gameState ) const;
  
  void PostSwitchState( SwitchStateType switchState, uint16_t gameObjectId ) const;
  
  void PostParameter( ParameterType parameter,
                      float parameterValue,
                      uint16_t gameObjectId,
                      int32_t timeInMilliSeconds,
                      CurveType curve ) const;
  
private:
  
  AudioEngineMessageHandler& _messageHandler;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  
  void HandleEvents(const AnkiEvent<MessageAudioClient>& event);
};

} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_AudioEngineClient_H__ */
