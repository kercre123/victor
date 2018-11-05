/**
* File: speechRecognizerSystem.h
*
* Author: Jordan Rivas
* Created: 10/23/2018
*
* Description: Speech Recognizer System handles high level speech features, such as, locale and multiple triggers
*
* Copyright: Anki, Inc. 2018
*
*/

#ifndef __AnimProcess_VictorAnim_SpeechRecognizerSystem_H_
#define __AnimProcess_VictorAnim_SpeechRecognizerSystem_H_

#include "audioUtil/speechRecognizer.h"
#include "cozmoAnim/micData/micTriggerConfig.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace Anki {
  namespace Vector {
    class AnimContext;
    namespace MicData {
      class MicDataSystem;
    }
    class RobotDataLoader;
    class SpeechRecognizerTHF;
  }
  namespace Util {
    class Locale;
  }
}

namespace Anki {
namespace Vector {

class SpeechRecognizerSystem
{
public:
  SpeechRecognizerSystem(const AnimContext* context,
                         MicData::MicDataSystem* micDataSystem,
                         const std::string& triggerWordDataDir);
  
  ~SpeechRecognizerSystem();
  
  SpeechRecognizerSystem(const SpeechRecognizerSystem& other) = delete;
  SpeechRecognizerSystem& operator=(const SpeechRecognizerSystem& other) = delete;
  
  void Init(const RobotDataLoader& dataLoader, const Util::Locale& locale);
  
  // Update recognizer audio
  // NOTE: Always call from tne same thread
  void Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen);
  
  // Set Default models for locale
  // Return true when locale file was found and is different then current locale
  // NOTE: Locale is not updated until the next Update() call
  bool UpdateTriggerForLocale(Util::Locale newLocale);
  
  // Trigger Callback types
  struct TriggerWordDetectedInfo {
    const char* result;
    float startTime_ms;
    float endTime_ms;
    float score;
  };
  using TriggerWordDetectedCallback = std::function<void(const TriggerWordDetectedInfo& info)>;
  
  // Assign callback for trigger detection
  void SetTriggerWordDetectedCallback(TriggerWordDetectedCallback callback) { _triggerCallback = callback; }


private:
  
  const AnimContext*                          _context = nullptr;
  MicData::MicDataSystem*                     _micDataSystem = nullptr;
  std::unique_ptr<SpeechRecognizerTHF>        _recognizer;
  std::unique_ptr<MicData::MicTriggerConfig>  _micTriggerConfig;
  TriggerWordDetectedCallback                 _triggerCallback = nullptr;
  std::string                                 _triggerWordDataDir;
  
  // For tracking and altering the trigger model being used
  MicData::MicTriggerConfig::TriggerDataPaths _currentTriggerPaths;
  MicData::MicTriggerConfig::TriggerDataPaths _nextTriggerPaths;
  std::mutex                                  _triggerModelMutex;
  std::atomic<bool>                           _isPendingLocaleUpdate{ false };
  
  // Handle callbacks from SpeechRecognizer
  void TriggerWordVoiceCallback(const char* resultFound, float score);
  
  // Set custom model and search files for locale
  // Return true when locale file was found and is different then current locale
  // NOTE: This only sets the _nextTriggerPaths the locale will be updated in the Update() call
  bool UpdateTriggerForLocale(Util::Locale newLocale,
                              MicData::MicTriggerConfig::ModelType modelType,
                              int searchFileIndex);
  
  // This should only be called from the Update() methods, needs to be performed on the same thread
  void ApplyLocaleUpdate();
  
  void SetupConsoleFuncs();
};


} // end namespace Vector
} // end namespace Anki

#endif // __AnimProcess_VictorAnim_SpeechRecognizerSystem_H_
