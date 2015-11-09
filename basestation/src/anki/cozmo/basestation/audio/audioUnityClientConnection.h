//
//  audioUnityClientCunnection.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/6/15.
//
//

#ifndef audioUnityClientCunnection_h
#define audioUnityClientCunnection_h

#include "anki/cozmo/basestation/audio/audioClientConnection.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class IExternalInterface;

template <typename Type>
class AnkiEvent;
  
namespace ExternalInterface {
class MessageGameToEngine;
}
  
namespace Audio {
  
class AudioUnityClientConnection : public AudioClientConnection {
  
public:
  AudioUnityClientConnection( IExternalInterface* externalInterface );
  ~AudioUnityClientConnection() override;
  
  void PostCallback( const AudioCallbackDuration& callbackMessage ) override;
  void PostCallback( const AudioCallbackMarker& callbackMessage ) override;
  void PostCallback( const AudioCallbackComplete& callbackMessage ) override;
  
private:
  IExternalInterface* _externalInterface;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
};


} // Audio
} // Cozmo
} // Anki

#endif /* audioUnityClientCunnection_h */
