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
, _recognizer(std::make_unique<SpeechRecognizerTHF>())
, _micTriggerConfig(std::make_unique<MicData::MicTriggerConfig>())
, _triggerWordDataDir(triggerWordDataDir)
{
  // Init Sensory processing. Note we don't add in a search trigger here, that happens later
  const std::string& pronunciationFileToUse = "";
  _recognizer->Init(pronunciationFileToUse);
  
  // Set up the callback that creates the recording job when the trigger is detected
  auto triggerCallback = std::bind(&SpeechRecognizerSystem::TriggerWordVoiceCallback,
                                   this, std::placeholders::_1, std::placeholders::_2);
  _recognizer->SetCallback(triggerCallback);
  _recognizer->Start();

  SetupConsoleFuncs();
  
  // TODO: Remove once we use _context to get ALEXA instance
  _context->GetRandom();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeechRecognizerSystem::~SpeechRecognizerSystem()
{
  _recognizer->Stop();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::Init(const RobotDataLoader& dataLoader, const Util::Locale& locale)
{
  _micTriggerConfig->Init(dataLoader.GetMicTriggerConfig());
  
  // On Debug builds, check that all the files listed in the trigger config actually exist
#if ANKI_DEVELOPER_CODE
  const auto& triggerDataList = _micTriggerConfig->GetAllTriggerModelFiles();
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
void SpeechRecognizerSystem::Update(const AudioUtil::AudioSample * audioData, unsigned int audioDataLen)
{
  // TODO: Add profiling for each recognizer
  if (_isPendingLocaleUpdate) {
    ApplyLocaleUpdate();
  }
  _recognizer->Update(audioData, audioDataLen);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeechRecognizerSystem::UpdateTriggerForLocale(Util::Locale newLocale)
{
  // Set local using defualt locale settings
  return UpdateTriggerForLocale(newLocale, MicData::MicTriggerConfig::ModelType::Count, -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::TriggerWordVoiceCallback(const char* resultFound, float score)
{
  if (_triggerCallback != nullptr) {
    TriggerWordDetectedInfo info {
      .result       = resultFound,
      .startTime_ms = 0.0f,
      .endTime_ms   = 0.0f,
      .score        = score
    };
    _triggerCallback(info);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeechRecognizerSystem::UpdateTriggerForLocale(Util::Locale newLocale,
                                                    MicData::MicTriggerConfig::ModelType modelType,
                                                    int searchFileIndex)
{
  std::lock_guard<std::mutex> lock(_triggerModelMutex);
  _nextTriggerPaths = _micTriggerConfig->GetTriggerModelDataPaths(newLocale, modelType, searchFileIndex);
  bool success = false;
  
  if (!_nextTriggerPaths.IsValid()) {
    LOG_WARNING("SpeechRecognizerSystem.UpdateTriggerForLocale.NoPathsFoundForLocale",
                "locale: %s modelType: %d searchFileIndex: %d",
                newLocale.ToString().c_str(), (int) modelType, searchFileIndex);
  }
  
  if (_currentTriggerPaths != _nextTriggerPaths) {
    _isPendingLocaleUpdate = true;
    success = true;
  }
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::ApplyLocaleUpdate()
{
  std::lock_guard<std::mutex> lock(_triggerModelMutex);
  if (_currentTriggerPaths != _nextTriggerPaths) {
    //  ANKI_CPU_PROFILE("SwitchTriggerWordSearch");  // TODO: Add Profiling
    _currentTriggerPaths = _nextTriggerPaths;
    _recognizer->SetRecognizerIndex(AudioUtil::SpeechRecognizer::InvalidIndex);
    const AudioUtil::SpeechRecognizer::IndexType singleSlotIndex = 0;
    _recognizer->RemoveRecognitionData(singleSlotIndex);
    
    if (_currentTriggerPaths.IsValid()) {
      const std::string& netFilePath = Util::FileUtils::FullFilePath( {_triggerWordDataDir,
        _currentTriggerPaths._dataDir,
        _currentTriggerPaths._netFile} );
      const std::string& searchFilePath = Util::FileUtils::FullFilePath( {_triggerWordDataDir,
        _currentTriggerPaths._dataDir,
        _currentTriggerPaths._searchFile} );
      const bool isPhraseSpotted = true;
      const bool allowsFollowUpRecog = false;
      const bool success = _recognizer->AddRecognitionDataFromFile(singleSlotIndex, netFilePath, searchFilePath,
                                                                   isPhraseSpotted, allowsFollowUpRecog);
      if (success) {
        LOG_INFO("SpeechRecognizerSystem.UpdateTriggerForLocale.SwitchTriggerSearch",
                 "Switched speechRecognizer to netFile: %s searchFile %s",
                 netFilePath.c_str(), searchFilePath.c_str());

        _recognizer->SetRecognizerIndex(singleSlotIndex);
      }
      else {
        _currentTriggerPaths = MicData::MicTriggerConfig::TriggerDataPaths{};
        _nextTriggerPaths = MicData::MicTriggerConfig::TriggerDataPaths{};
        LOG_ERROR("SpeechRecognizerSystem.UpdateTriggerForLocale.FailedSwitchTriggerSearch",
                  "Failed to add speechRecognizer netFile: %s searchFile %s",
                  netFilePath.c_str(), searchFilePath.c_str());
      }
    }
    else {
      LOG_INFO("SpeechRecognizerSystem.UpdateTriggerForLocale.ClearTriggerSearch",
               "Cleared speechRecognizer to have no search");
    }
  }
  _isPendingLocaleUpdate = false;
}

} // end namespace Vector
} // end namespace Anki
