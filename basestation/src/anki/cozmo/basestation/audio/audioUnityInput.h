/*
 * File: audioUnityInput.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This Connection is specific to communicating with the Unity Message Interface. It is a subclass of
 *              AudioMuxInput.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_AudioUnityInput_H__
#define __Basestation_Audio_AudioUnityInput_H__

#include "audioEngine/multiplexer/audioMuxInput.h"
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

class AudioUnityInput : public AudioEngine::Multiplexer::AudioMuxInput {

public:

  AudioUnityInput( IExternalInterface& externalInterface );

  void PostCallback( const AudioEngine::Multiplexer::AudioCallback& callbackMessage ) const override;
  

private:
  
  IExternalInterface& _externalInterface;
  std::vector<Signal::SmartHandle> _signalHandles;

  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);

};


} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_AudioUnityInput_H__ */
