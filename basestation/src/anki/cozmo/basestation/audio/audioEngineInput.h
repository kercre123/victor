/*
 * File: audioEngineInput.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is a subclass of AudioMuxInput which provides communication between itself and an
 *              AudioEngineClient by means of AudioEngineMessageHandler.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_AudioEngineInput_H__
#define __Basestation_Audio_AudioEngineInput_H__

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "audioEngine/multiplexer/audioMuxInput.h"
#include "util/signals/signalHolder.h"
#include <vector>


namespace Anki {
namespace AudioEngine {
namespace Multiplexer {
class MessageAudioClient;
}
}

namespace Cozmo {
namespace Audio {
  

class AudioEngineMessageHandler;
  
class AudioEngineInput : public AudioEngine::Multiplexer::AudioMuxInput, protected Anki::Util::SignalHolder {
  
public:
  // Transfer Message Handler Ownership
  AudioEngineInput( AudioEngineMessageHandler* messageHandler );
  virtual ~AudioEngineInput() override;
  
  AudioEngineMessageHandler* GetMessageHandler() const { return _messageHandler; }
  
  virtual void PostCallback( const AudioEngine::Multiplexer::AudioCallback& callbackMessage ) const override;
  
  
private:
  
  AudioEngineMessageHandler* _messageHandler;
  
  void HandleEvents(const AnkiEvent<AudioEngine::Multiplexer::MessageAudioClient>& event);
  
};


} // Audio
} // Cozmo
} // Anki


#endif /* __Basestation_Audio_AudioEngineInput_H__ */
