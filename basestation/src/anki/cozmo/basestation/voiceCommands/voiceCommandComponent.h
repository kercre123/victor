/**
* File: voiceCommandComponent.h
*
* Author: Lee Crippen
* Created: 2/8/2017
*
* Description: Component handling listening for voice commands, controlling recognizer
* listen context, and broadcasting command events.
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __Cozmo_Basestation_VoiceCommands_VoiceCommandComponent_H_
#define __Cozmo_Basestation_VoiceCommands_VoiceCommandComponent_H_

#include "anki/cozmo/basestation/components/bodyLightComponentTypes.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "audioUtil/audioCaptureSystem.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/signalHolder.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include <memory>
#include <mutex>

namespace Anki {
  
namespace AudioUtil {
  class AudioRecognizerProcessor;
  class SpeechRecognizer;
}
  
namespace Util {
namespace Data {
  class DataPlatform;
}
}
  
namespace Cozmo {

template <typename T> class AnkiEvent;
namespace ExternalInterface {
  class MessageGameToEngine;
}
  
namespace VoiceCommand {

class CommandPhraseData;


class VoiceCommandComponent : private Util::noncopyable, private Util::SignalHolder
{
public:
  VoiceCommandComponent(const CozmoContext& context);
  virtual ~VoiceCommandComponent();
  
  void Update();
  
  bool KeyPhraseWasHeard() const { return _pendingHeardCommand == VoiceCommandType::HeyCozmo; }
  bool AnyCommandPending() const { return _pendingHeardCommand != VoiceCommandType::Count; }
  VoiceCommandType GetPendingCommand() const { return _pendingHeardCommand; }
  
  void ClearHeardCommand() { _pendingHeardCommand = VoiceCommandType::Count; }
  
  void SetListenContext(VoiceCommandListenContext listenContext);
  void DoForceHeardPhrase(VoiceCommandType commandType);
  
  template<typename T>
  void HandleMessage(const T& msg);
  
  template<typename T>
  void BroadcastVoiceEvent(T&& event, bool useDeferred = false);
  
private:
  const CozmoContext&                                   _context;
  std::unique_ptr<AudioUtil::AudioRecognizerProcessor>  _recogProcessor;
  std::unique_ptr<AudioUtil::AudioCaptureSystem>        _captureSystem;
  std::unique_ptr<AudioUtil::SpeechRecognizer>          _recognizer;
  std::unique_ptr<CommandPhraseData>                    _phraseData;
  BackpackLightDataLocator                              _bodyLightDataLocator{};
  float                                                 _commandLightTimeRemaining_s = -1.f;
  VoiceCommandListenContext                             _listenContext = VoiceCommandListenContext::Keyphrase;
  VoiceCommandType                                      _pendingHeardCommand = VoiceCommandType::Count;
  bool                                                  _initialized = false;
  bool                                                  _commandRecogEnabled = false;
  bool                                                  _recordPermissionBeingRequested = false;
  bool                                                  _permRequestAlreadyDenied = false;
  std::recursive_mutex                                  _permissionCallbackLock;
  
  void Init();
  
  // Updates the status of the backpack light on Cozmo that indicates hearing a command
  void UpdateCommandLight(bool heardTriggerPhrase);
  bool HandleCommand(const VoiceCommandType& command);
  
  void ResetContext();
  bool RequestEnableVoiceCommand(AudioUtil::AudioCaptureSystem::PermissionState permissionState);
  
  AudioCapturePermissionState ConvertAudioCapturePermission(AudioUtil::AudioCaptureSystem::PermissionState state);
  bool StateRequiresCallback(AudioUtil::AudioCaptureSystem::PermissionState permissionState) const;
  
}; // class VoiceCommandComponent
  

template<typename T>
void VoiceCommandComponent::BroadcastVoiceEvent(T&& event, bool useDeferred)
{
  auto* externalInterface = _context.GetExternalInterface();
  if (externalInterface)
  {
    if (useDeferred)
    {
      externalInterface->BroadcastDeferred(ExternalInterface::MessageEngineToGame(VoiceCommandEvent(VoiceCommandEventUnion(std::forward<T>(event)))));
    }
    else
    {
      externalInterface->BroadcastToGame<VoiceCommandEvent>(VoiceCommandEventUnion(std::forward<T>(event)));
    }
  }
}

  
} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_VoiceCommandComponent_H_
