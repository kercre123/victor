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
#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

#include "anki/cozmo/basestation/components/bodyLightComponentTypes.h"
#include "util/helpers/noncopyable.h"
#include "clad/types/voiceCommandTypes.h"

#include <memory>

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

class CommandPhraseData;
class CozmoContext;

class VoiceCommandComponent : private Util::noncopyable
{
public:
  VoiceCommandComponent(const CozmoContext& context);
  virtual ~VoiceCommandComponent();
  
  void Init();
  
  void Update();
  
  bool KeyPhraseWasHeard() const { return _pendingHeardCommand == VoiceCommandType::HeyCozmo; }
  bool AnyCommandPending() const { return _pendingHeardCommand != VoiceCommandType::Count; }
  VoiceCommandType GetPendingCommand() const { return _pendingHeardCommand; }
  
  void ClearHeardCommand() { _pendingHeardCommand = VoiceCommandType::Count; }
  
  void SetListenContext(VoiceCommandListenContext listenContext) { _listenContext = listenContext; }
  
private:
  const CozmoContext&                                   _context;
  std::unique_ptr<AudioUtil::AudioRecognizerProcessor>  _recogProcessor;
  std::unique_ptr<AudioUtil::SpeechRecognizer>          _recognizer;
  std::unique_ptr<CommandPhraseData>                    _phraseData;
  BackpackLightDataLocator                              _bodyLightDataLocator{};
  float                                                 _commandLightTimeRemaining_s = -1.f;
  VoiceCommandListenContext                             _listenContext = VoiceCommandListenContext::Keyphrase;
  VoiceCommandType                                      _pendingHeardCommand = VoiceCommandType::Count;
  
  template<typename T>
  void BroadcastVoiceEvent(T&& event);
  
  // Updates the status of the backpack light on Cozmo that indicates hearing a command
  void UpdateCommandLight(bool heardTriggerPhrase);
  bool HandleCommand(const VoiceCommandType& command);
  
}; // class VoiceCommandComponent


} // namespace Cozmo
} // namespace Anki

#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
#endif // __Cozmo_Basestation_VoiceCommands_VoiceCommandComponent_H_
