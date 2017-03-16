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
#include "util/helpers/noncopyable.h"

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
  
class CozmoContext;

#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
class VoiceCommandComponent : private Util::noncopyable
{
public:
  VoiceCommandComponent(const CozmoContext& context);
  virtual ~VoiceCommandComponent();
  
  void Update();
  
private:
  const CozmoContext&                                   _context;
  const Util::Data::DataPlatform&                       _platform;
  std::unique_ptr<AudioUtil::AudioRecognizerProcessor>  _recogProcessor;
  std::unique_ptr<AudioUtil::SpeechRecognizer>          _recognizer;
  BackpackLightDataLocator                              _bodyLightDataLocator{};
  float                                                 _commandLightTimeRemaining_s = -1.f;
  
  template<typename T>
  void BroadcastVoiceEvent(T&& event);
  
  // Updates the status of the backpack light on Cozmo that indicates hearing a command
  void UpdateCommandLight(bool heardTriggerPhrase);
  
}; // class VoiceCommandComponent

#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_VoiceCommandComponent_H_
