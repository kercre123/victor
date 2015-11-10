/*
 * File: audioUnityClientConnection.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This Connection is specific to communicating with the Unity Message Interface. It is a sub-class of
 *              AudioClientConnection.
 *
 * Copyright: Anki, Inc. 2015
 */

#ifndef __Basestation_Audio_AudioUnityClientCunnection_H__
#define __Basestation_Audio_AudioUnityClientCunnection_H__

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
  
  AudioUnityClientConnection( IExternalInterface& externalInterface );
  
  void PostCallback( const AudioCallbackDuration& callbackMessage ) override;
  void PostCallback( const AudioCallbackMarker& callbackMessage ) override;
  void PostCallback( const AudioCallbackComplete& callbackMessage ) override;
  
private:
  IExternalInterface& _externalInterface;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  void HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
};


} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_AudioUnityClientCunnection_H__ */
