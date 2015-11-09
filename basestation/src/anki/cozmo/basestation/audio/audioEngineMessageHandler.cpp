//
//  audioEngineMessageHandler.cpp
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/8/15.
//
//

#include "anki/cozmo/basestation/audio/audioEngineMessageHandler.h"


namespace Anki {
namespace Cozmo {
namespace Audio {

void AudioEngineMessageHandler::Broadcast( const MessageAudioClient& message)
{
  _eventMgr.Broadcast( AnkiEvent<MessageAudioClient>( static_cast<uint32_t>(message.GetTag()), message ));
}


Signal::SmartHandle AudioEngineMessageHandler::Subscribe(const MessageAudioClientTag& tagType, std::function<void(const AnkiEvent<MessageAudioClient>&)> messageHandler)
{
  return _eventMgr.Subcribe( static_cast<uint32_t>(tagType), messageHandler );
}

  
} // Audio
} // Cozmo
} // Anki
