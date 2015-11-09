//
//  audioEngineClientCunnection.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/8/15.
//
//

#ifndef audioEngineClientCunnection_h
#define audioEngineClientCunnection_h

#include "anki/cozmo/basestation/audio/audioClientConnection.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "util/signals/simpleSignal.hpp"
#include <vector>


namespace Anki {
namespace Cozmo {
namespace Audio {
  
class MessageAudioClient;
class AudioEngineMessageHandler;
  
class AudioEngineClientConnection : public AudioClientConnection {
  
public:
  // Transfer Message Handler Ownership
  AudioEngineClientConnection( AudioEngineMessageHandler* messageHandler );
  ~AudioEngineClientConnection() override;
  
  AudioEngineMessageHandler* GetMessageHandler() const { return _messageHandler; }
  
  void PostCallback( const AudioCallbackDuration& callbackMessage ) override;
  void PostCallback( const AudioCallbackMarker& callbackMessage ) override;
  void PostCallback( const AudioCallbackComplete& callbackMessage ) override;
  
  
  
private:
  
  AudioEngineMessageHandler* _messageHandler;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  
  void HandleEvents(const AnkiEvent<MessageAudioClient>& event);
  
};


} // Audio
} // Cozmo
} // Anki


#endif /* audioEngineClientCunnection_h */
