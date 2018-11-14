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
    class Alexa;
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
  
  using TriggerWordDetectedCallback = std::function<void(const AudioUtil::SpeechRecognizer::SpeechCallbackInfo& info)>;
  
  // Init Vector trigger detector
  // Note: This always happens at boot
  void InitVector(const RobotDataLoader& dataLoader,
                  const Util::Locale& locale,
                  TriggerWordDetectedCallback callback);
  
  // Init Alexa trigger detector
  // Note: This is done after Alex user has been authicated
  void InitAlexa(const RobotDataLoader& dataLoader,
                 const Util::Locale& locale,
                 TriggerWordDetectedCallback callback);
  
  // Init Alex playback trigger detector
  // Run trigger detection on playback audio
  // Note: This is done after Alex user has been authicated
  void InitAlexaPlayback(const RobotDataLoader& dataLoader,
                         TriggerWordDetectedCallback callback);
  
  // Disable Alexa trigger
  void DisableAlexa();
  
  // Update recognizer audio
  // NOTE: Always call from tne same thread
  void Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen, bool vadActive);
  
  // Set Default models for locale
  // Return true when locale file was found and is different then current locale
  // NOTE: Locale is not updated until the next Update() call
  bool UpdateTriggerForLocale(const Util::Locale newLocale);
  
  // HACK: Test using a detector on playback audio
  // Return null when recognizer
  SpeechRecognizerTHF* GetAlexaPlaybackRecognizer();


private:
  
  // Trigger context
  struct TriggerContext {
    std::unique_ptr<SpeechRecognizerTHF>        recognizer;
    std::unique_ptr<MicData::MicTriggerConfig>  micTriggerConfig;
    // For tracking and altering the trigger model being used
    MicData::MicTriggerConfig::TriggerDataPaths currentTriggerPaths;
    MicData::MicTriggerConfig::TriggerDataPaths nextTriggerPaths;
    
    TriggerContext();
  };
  
  const AnimContext*                          _context = nullptr;
  MicData::MicDataSystem*                     _micDataSystem = nullptr;
  std::unique_ptr<TriggerContext>             _victorTrigger;
  std::unique_ptr<TriggerContext>             _alexaTrigger;
  Alexa*                                      _alexaComponent = nullptr;
  std::unique_ptr<TriggerContext>             _alexaPlaybackTrigger;
  std::string                                 _triggerWordDataDir;
  
  std::mutex                                  _triggerModelMutex;
  std::atomic<bool>                           _isPendingLocaleUpdate{ false };
  
  // Set custom model and search files for locale
  // Return true when locale file was found and is different then current locale
  // NOTE: This only sets the _nextTriggerPaths the locale will be updated in the Update() call
  bool UpdateTriggerForLocale(TriggerContext& trigger,
                              const Util::Locale newLocale,
                              const MicData::MicTriggerConfig::ModelType modelType,
                              const int searchFileIndex);
  
  // This should only be called from the Update() methods, needs to be performed on the same thread
  void ApplyLocaleUpdate();
  
  // Console Var methods
  // NOTE: These methods provide no functionality when ANKI_DEV_CHEATS is turned off
  void SetupConsoleFuncs();
  std::string UpdateRecognizerHelper(size_t& inOut_modelIdx, size_t new_modelIdx,
                                     int& inOut_searchIdx, int new_searchIdx,
                                     SpeechRecognizerSystem::TriggerContext& trigger);
};


} // end namespace Vector
} // end namespace Anki

#endif // __AnimProcess_VictorAnim_SpeechRecognizerSystem_H_
