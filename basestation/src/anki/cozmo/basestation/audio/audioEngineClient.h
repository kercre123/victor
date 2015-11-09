//
//  audioEngineClient.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/8/15.
//
//

#ifndef audioEngineClient_h
#define audioEngineClient_h

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "util/signals/simpleSignal.hpp"
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

class AudioEngineClient
{
public:
  
  AudioEngineClient( AudioEngineMessageHandler* messageHandler );
  ~AudioEngineClient();
  
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
  
  AudioEngineMessageHandler* _messageHandler;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  
  void HandleEvents(const AnkiEvent<MessageAudioClient>& event);
};

} // Audio
} // Cozmo
} // Anki

#endif /* audioEngineClient_h */
