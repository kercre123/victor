/**
 * File: speechRecognizerSystem.cpp
 *
 * Author: Jordan Rivas
 * Created: 10/23/2018
 *
 * Description: Speech Recognizer System handles high level speech features, such as, locale and multiple triggers
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#include "cozmoAnim/speechRecognizer/speechRecognizerSystem.h"

#include "cozmoAnim/alexa/alexa.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/robotDataLoader.h"
#include "cozmoAnim/speechRecognizer/speechRecognizerTHFSimple.h"
#include "util/console/consoleInterface.h"
#include "util/console/consoleFunction.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/environment/locale.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include <list>


namespace Anki {
namespace Vector {

namespace {
#define LOG_CHANNEL "SpeechRecognizer"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Console Vars
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if ANKI_DEV_CHEATS
#define CONSOLE_GROUP "SpeechRecognizer"
  
// NOTE: This enum needs to EXACTLY match the number and ordering of the kTriggerModelDataList array below
enum class SupportedLocales
{
  enUS_1mb, // default
  enUS_500kb,
  enUS_250kb,
  enUS_Alt_1mb,
  enUK,
  enAU,
  frFR,
  deDE,
  Count
};

using MicConfigModelType = MicData::MicTriggerConfig::ModelType;
struct TriggerModelTypeData
{
  Util::Locale          locale;
  MicConfigModelType    modelType;
  int                   searchFileIndex;
};

// NOTE: This array needs to EXACTLY match the number and ordering of the SupportedLocales enum above
const TriggerModelTypeData kTriggerModelDataList[] =
{
  // Easily selectable values for consolevar dropdown. Note 'Count' and '-1' values indicate to use default
  // We are using delivery 1 as our defualt enUS model
  { .locale = Util::Locale("en","US"), .modelType = MicConfigModelType::size_1mb, .searchFileIndex = -1 },
  { .locale = Util::Locale("en","US"), .modelType = MicConfigModelType::size_500kb, .searchFileIndex = -1 },
  { .locale = Util::Locale("en","US"), .modelType = MicConfigModelType::size_250kb, .searchFileIndex = -1 },
  // This is a hack to add a second en_US model, it will appear in console vars as `enUS_Alt_1mb`
  // This is delivery 2 model
  { .locale = Util::Locale("en","ZW"), .modelType = MicConfigModelType::size_1mb, .searchFileIndex = -1 },
  // Other Locales
  { .locale = Util::Locale("en","GB"), .modelType = MicConfigModelType::Count, .searchFileIndex = -1 },
  { .locale = Util::Locale("en","AU"), .modelType = MicConfigModelType::Count, .searchFileIndex = -1 },
  { .locale = Util::Locale("fr","FR"), .modelType = MicConfigModelType::Count, .searchFileIndex = -1 },
  { .locale = Util::Locale("de","DE"), .modelType = MicConfigModelType::Count, .searchFileIndex = -1 },
};
constexpr size_t kTriggerDataListLen = sizeof(kTriggerModelDataList) / sizeof(kTriggerModelDataList[0]);
static_assert(kTriggerDataListLen == (size_t) SupportedLocales::Count, "Need trigger data for each supported locale");

size_t _recognizerModelTypeIndex = (size_t) SupportedLocales::enUS_500kb;
CONSOLE_VAR_ENUM(size_t, kRecognizerModel, CONSOLE_GROUP, _recognizerModelTypeIndex,
                 "enUS_1mb,enUS_500kb,enUS_250kb,enUS_Alt_1mb,enUK,enAU,frFR,deDE");

int _triggerModelSensitivityIndex = 0;
CONSOLE_VAR_ENUM(int, kRecognizerModelSensitivity, CONSOLE_GROUP, _triggerModelSensitivityIndex,
                 "default,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20");

std::list<Anki::Util::IConsoleFunction> sConsoleFuncs;

#endif // ANKI_DEV_CHEATS
} // namespace

void SpeechRecognizerSystem::SetupConsoleFuncs()
{
#if ANKI_DEV_CHEATS
  // Update Recognizer Model with kRecognizerModel & kRecognizerModelSensitivity enums
  auto updateRecognizerModel = [this](ConsoleFunctionContextRef context) {
    if ((_recognizerModelTypeIndex != kRecognizerModel) ||
        (_triggerModelSensitivityIndex != kRecognizerModelSensitivity))
    {
      _recognizerModelTypeIndex = kRecognizerModel;
      _triggerModelSensitivityIndex = kRecognizerModelSensitivity;
      const auto& newTypeData = kTriggerModelDataList[kRecognizerModel];
      _micDataSystem->SetLocaleDevOnly(newTypeData.locale);
      const int sensitivitySearchFileIdx = (kRecognizerModelSensitivity == 0) ?
                                             newTypeData.searchFileIndex : kRecognizerModelSensitivity;
      const bool success = UpdateTriggerForLocale(newTypeData.locale,
                                                  newTypeData.modelType,
                                                  sensitivitySearchFileIdx);
      context->channel->WriteLog("UpdateRecognizerModel %s", (success ? "success!" : "fail :("));
    }
  };

  sConsoleFuncs.emplace_front("UpdateRecognizerModel", std::move(updateRecognizerModel), CONSOLE_GROUP, "");
#endif
  _micDataSystem->GetSpeakerLatency_ms(); // Fix compiler error when ANKI_DEV_CHEATS is not enabled
}

# undef CONSOLE_GROUP
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SpeechRecognizerSystem
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeechRecognizerSystem::SpeechRecognizerSystem(const AnimContext* context,
                                               MicData::MicDataSystem* micDataSystem,
                                               const std::string& triggerWordDataDir)
: _context(context)
, _micDataSystem(micDataSystem)
, _triggerWordDataDir(triggerWordDataDir)
{
  SetupConsoleFuncs();
  
  // TODO: Remove once we use _context to get ALEXA instance
  _context->GetRandom();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeechRecognizerSystem::~SpeechRecognizerSystem()
{
  _victorTrigger->recognizer->Stop();
  if (_alexaTrigger) {
    _alexaTrigger->recognizer->Stop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::InitVector(const RobotDataLoader& dataLoader,
                                        const Util::Locale& locale,
                                        TriggerWordDetectedCallback callback)
{
  if (_victorTrigger) {
    LOG_WARNING("SpeechRecognizerSystem.InitVector", "Victor Recognizer is already running");
    return;
  }
  
  _victorTrigger = std::make_unique<TriggerContext>();
  _victorTrigger->recognizer->Init("");
  _victorTrigger->recognizer->SetCallback(callback);
  _victorTrigger->recognizer->Start();
  _victorTrigger->micTriggerConfig->Init(dataLoader.GetMicTriggerConfig());
  
  // On Debug builds, check that all the files listed in the trigger config actually exist
#if ANKI_DEVELOPER_CODE
  const auto& triggerDataList = _victorTrigger->micTriggerConfig->GetAllTriggerModelFiles();
  for (const auto& filePath : triggerDataList) {
    const auto& fullFilePath = Util::FileUtils::FullFilePath( {_triggerWordDataDir, filePath} );
    if (Util::FileUtils::FileDoesNotExist(fullFilePath)) {
      LOG_WARNING("SpeechRecognizerSystem.Init.MicTriggerConfigFileMissing","%s",fullFilePath.c_str());
    }
  }
#endif // ANKI_DEVELOPER_CODE
  
  UpdateTriggerForLocale(locale);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::InitAlexa(const RobotDataLoader& dataLoader,
                                       const Util::Locale& locale,
                                       TriggerWordDetectedCallback callback)
{
  // This called when Alexa is authorized
  if (_alexaTrigger) {
    LOG_WARNING("SpeechRecognizerSystem.InitAlexa", "Alexa Recognizer is already running");
    return;
  }
  
  _alexaComponent = _context->GetAlexa();
  ASSERT_NAMED(_alexaComponent != nullptr, "SpeechRecognizerSystem.InitAlexa._context.GetAlexa.IsNull");
  _alexaTrigger = std::make_unique<TriggerContext>();
  _alexaTrigger->recognizer->Init("");
  _alexaTrigger->recognizer->SetCallback(callback);
  _alexaTrigger->recognizer->Start();
  
  // TODO: VIC-11301 Add Alexa mic trigger config
  _isPendingLocaleUpdate = true;
  /*
  _alexaTrigger->micTriggerConfig->Init(dataLoader.GetMicTriggerConfig());
  
  // On Debug builds, check that all the files listed in the trigger config actually exist
#if ANKI_DEVELOPER_CODE
  const auto& triggerDataList = _alexaTrigger->micTriggerConfig->GetAllTriggerModelFiles();
  for (const auto& filePath : triggerDataList) {
    const auto& fullFilePath = Util::FileUtils::FullFilePath( {_triggerWordDataDir, filePath} );
    if (Util::FileUtils::FileDoesNotExist(fullFilePath)) {
      LOG_WARNING("SpeechRecognizerSystem.Init.MicTriggerConfigFileMissing","%s",fullFilePath.c_str());
    }
  }
#endif // ANKI_DEVELOPER_CODE
   */
  
  UpdateTriggerForLocale(locale);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::DisableAlexa()
{
  if (_alexaTrigger) {
    _alexaTrigger->recognizer->Stop();
    _alexaTrigger.reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen, bool vadActive)
{
  // TODO: Add profiling for each recognizer
  if (_isPendingLocaleUpdate) {
    ApplyLocaleUpdate();
  }
  // Update recognizer
  if (vadActive) {
    _victorTrigger->recognizer->Update(audioData, audioDataLen);
    if (_alexaTrigger) {
      _alexaTrigger->recognizer->Update(audioData, audioDataLen);
    }
  }
  // Send all audio to Alexa
  if (_alexaComponent != nullptr) {
    _alexaComponent->AddMicrophoneSamples(audioData, audioDataLen);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeechRecognizerSystem::UpdateTriggerForLocale(Util::Locale newLocale)
{
  // Set local using defualt locale settings
  return UpdateTriggerForLocale(newLocale, MicData::MicTriggerConfig::ModelType::Count, -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeechRecognizerSystem::UpdateTriggerForLocale(Util::Locale newLocale,
                                                    MicData::MicTriggerConfig::ModelType modelType,
                                                    int searchFileIndex)
{
  std::lock_guard<std::mutex> lock(_triggerModelMutex);
  _victorTrigger->nextTriggerPaths = _victorTrigger->micTriggerConfig->GetTriggerModelDataPaths(newLocale,
                                                                                                modelType,
                                                                                                searchFileIndex);
  bool success = false;
  
  if (!_victorTrigger->nextTriggerPaths.IsValid()) {
    LOG_WARNING("SpeechRecognizerSystem.UpdateTriggerForLocale.NoPathsFoundForLocale",
                "locale: %s modelType: %d searchFileIndex: %d",
                newLocale.ToString().c_str(), (int) modelType, searchFileIndex);
  }
  
  if (_victorTrigger->currentTriggerPaths != _victorTrigger->nextTriggerPaths) {
    _isPendingLocaleUpdate = true;
    success = true;
  }
  
  // TODO: ADD ALEXA
  // TODO: VIC-11301 Add Alexa mic trigger config
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::ApplyLocaleUpdate()
{
  std::lock_guard<std::mutex> lock(_triggerModelMutex);
  
  // TODO: ADD ALEXA
  TriggerContext* triggers[] = { _victorTrigger.get() };
  size_t triggerCount = 1;
  for (size_t idx = 0; idx < triggerCount; ++idx) {    
    SpeechRecognizerTHF* recognizer = triggers[idx]->recognizer.get();
    MicData::MicTriggerConfig::TriggerDataPaths &currentTrigPathRef = triggers[idx]->currentTriggerPaths;
    MicData::MicTriggerConfig::TriggerDataPaths &nextTrigPathRef    = triggers[idx]->nextTriggerPaths;
    
    if (currentTrigPathRef != nextTrigPathRef) {
      //  ANKI_CPU_PROFILE("SwitchTriggerWordSearch");  // TODO: Add Profiling
      currentTrigPathRef = nextTrigPathRef;
      recognizer->SetRecognizerIndex(AudioUtil::SpeechRecognizer::InvalidIndex);
      const AudioUtil::SpeechRecognizer::IndexType singleSlotIndex = 0;
      recognizer->RemoveRecognitionData(singleSlotIndex);
      
      if (currentTrigPathRef.IsValid()) {
        const std::string& netFilePath = Util::FileUtils::FullFilePath( {_triggerWordDataDir,
          currentTrigPathRef._dataDir,
          currentTrigPathRef._netFile} );
        const std::string& searchFilePath = Util::FileUtils::FullFilePath( {_triggerWordDataDir,
          currentTrigPathRef._dataDir,
          currentTrigPathRef._searchFile} );
        const bool isPhraseSpotted = true;
        const bool allowsFollowUpRecog = false;
        const bool success = recognizer->AddRecognitionDataFromFile(singleSlotIndex, netFilePath, searchFilePath,
                                                                    isPhraseSpotted, allowsFollowUpRecog);
        if (success) {
          LOG_INFO("SpeechRecognizerSystem.UpdateTriggerForLocale.SwitchTriggerSearch",
                   "Switched speechRecognizer to netFile: %s searchFile %s",
                   netFilePath.c_str(), searchFilePath.c_str());
          
          recognizer->SetRecognizerIndex(singleSlotIndex);
        }
        else {
          currentTrigPathRef = MicData::MicTriggerConfig::TriggerDataPaths{};
          nextTrigPathRef = MicData::MicTriggerConfig::TriggerDataPaths{};
          LOG_WARNING("SpeechRecognizerSystem.UpdateTriggerForLocale.FailedSwitchTriggerSearch",
                    "Failed to add speechRecognizer netFile: %s searchFile %s",
                    netFilePath.c_str(), searchFilePath.c_str());
        }
      }
      else {
        LOG_WARNING("SpeechRecognizerSystem.UpdateTriggerForLocale.ClearTriggerSearch",
                 "Cleared speechRecognizer to have no search");
      }
    }
  }
  
  // TODO: VIC-11301 Add Alexa mic trigger config
  if (_alexaTrigger) {
    _alexaTrigger->recognizer->SetRecognizerIndex(AudioUtil::SpeechRecognizer::InvalidIndex);
    const AudioUtil::SpeechRecognizer::IndexType singleSlotIndex = 0;
    _alexaTrigger->recognizer->RemoveRecognitionData(singleSlotIndex);
    
    // This a hack to load trigger words
    const std::string& netFilePath = Util::FileUtils::FullFilePath( {_triggerWordDataDir,
      "alexa/thfft_alexa_enus_v3_500kb",
      "thfft_alexa_enus_v3_500kb_am.raw" } );
    const std::string& searchFilePath = Util::FileUtils::FullFilePath( {_triggerWordDataDir,
      "alexa/thfft_alexa_enus_v3_500kb",
      "thfft_alexa_enus_v3_500kb_search_11.raw"} );
    const bool isPhraseSpotted = true;
    const bool allowsFollowUpRecog = false;
    const bool success = _alexaTrigger->recognizer->AddRecognitionDataFromFile(singleSlotIndex,
                                                                               netFilePath,
                                                                               searchFilePath,
                                                                               isPhraseSpotted,
                                                                               allowsFollowUpRecog);
    if (success) {
      LOG_INFO("SpeechRecognizerSystem.UpdateTriggerForLocale.SwitchTriggerSearch.Alexa",
               "Switched speechRecognizer to netFile: %s searchFile %s",
               netFilePath.c_str(), searchFilePath.c_str());
      _alexaTrigger->recognizer->SetRecognizerIndex(singleSlotIndex);
    }
    else {
      LOG_WARNING("SpeechRecognizerSystem.UpdateTriggerForLocale.FailedSwitchTriggerSearch.Alexa",
                  "Failed to add speechRecognizer netFile: %s searchFile %s",
                  netFilePath.c_str(), searchFilePath.c_str());
    }
  }
  
  _isPendingLocaleUpdate = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeechRecognizerSystem::TriggerContext::TriggerContext()
: recognizer(std::make_unique<SpeechRecognizerTHF>())
, micTriggerConfig(std::make_unique<MicData::MicTriggerConfig>())
{
}


} // end namespace Vector
} // end namespace Anki
