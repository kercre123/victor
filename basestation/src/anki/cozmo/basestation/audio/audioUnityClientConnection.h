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
  ~AudioUnityClientConnection();
  
private:
  IExternalInterface* _externalInterface;
  
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
};


} // Audio
} // Cozmo
} // Anki

#endif /* audioUnityClientCunnection_h */
