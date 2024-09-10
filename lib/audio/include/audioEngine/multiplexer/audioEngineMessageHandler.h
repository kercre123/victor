/*
 * File: audioEngineMessageHandler.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is an event handler for Broadcasting and Subscribing to Audio CLAD Messages through MessageAudioClient Message.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_AudioEngineMessageHandler_H__
#define __Basestation_Audio_AudioEngineMessageHandler_H__

#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "clad/audio/messageAudioClient.h"
#include "util/signals/simpleSignal_fwd.h"
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

#endif /* __Basestation_Audio_AudioEngineMessageHandler_H__ */
