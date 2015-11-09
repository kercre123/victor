//
//  audioEngineMessageHandler.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/8/15.
//
//

#ifndef audioEngineMessageHandler_h
#define audioEngineMessageHandler_h

#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "clad/audio/messageAudioClient.h"
#include "util/signals/simpleSignal.hpp"
#include <vector>
#include <utility>


namespace Anki {
namespace Cozmo {
namespace Audio {

  
class AudioEngineMessageHandler
{
public:
  
  void Broadcast( const MessageAudioClient& message);
  
  template<typename T, typename ...Args>
  void BroadcastToAudioClient(Args&& ...args)
  {
    Broadcast(MessageAudioClient(T(std::forward<Args>(args)...)));
  }
  
  Signal::SmartHandle Subscribe(const MessageAudioClientTag& tagType, std::function<void(const AnkiEvent<MessageAudioClient>&)> messageHandler);
  
private:
  
  AnkiEventMgr<MessageAudioClient> _eventMgr;
  
};
  
} // Audio
} // Cozmo
} // Anki

#endif /* audioEngineMessageHandler_h */
