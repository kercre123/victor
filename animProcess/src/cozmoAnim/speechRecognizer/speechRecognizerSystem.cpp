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

#include "audioUtil/speechRecognizer.h"
#include "cozmoAnim/alexa/alexa.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/robotDataLoader.h"
#include "cozmoAnim/speechRecognizer/speechRecognizerTHFSimple.h"
#include "cozmoAnim/micData/notchDetector.h"
#include "util/console/consoleInterface.h"
#include "util/console/consoleFunction.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/environment/locale.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include <list>

#include <fcntl.h>
#include <unistd.h>


namespace Anki {
namespace Vector {

namespace {
#define LOG_CHANNEL "SpeechRecognizer"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Console Vars
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if ANKI_DEV_CHEATS
#define CONSOLE_GROUP_VECTOR "SpeechRecognizer.Vector"
#define CONSOLE_GROUP_ALEXA "SpeechRecognizer.Alexa"
  
// NOTE: This enum needs to EXACTLY match the number and ordering of the kTriggerModelDataList array below
enum class SupportedLocales
{
  enUS_1mb, // default
  enUS_500kb,
  enUS_250kb,
  enUS_Alt_1mb,
  enUS_Alt_500kb,
  enUS_Alt_250kb,
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
  { .locale = Util::Locale("en","ZW"), .modelType = MicConfigModelType::size_500kb, .searchFileIndex = -1 },
  { .locale = Util::Locale("en","ZW"), .modelType = MicConfigModelType::size_250kb, .searchFileIndex = -1 },
  // Other Locales
  { .locale = Util::Locale("en","GB"), .modelType = MicConfigModelType::Count, .searchFileIndex = -1 },
  { .locale = Util::Locale("en","AU"), .modelType = MicConfigModelType::Count, .searchFileIndex = -1 },
  { .locale = Util::Locale("fr","FR"), .modelType = MicConfigModelType::Count, .searchFileIndex = -1 },
  { .locale = Util::Locale("de","DE"), .modelType = MicConfigModelType::Count, .searchFileIndex = -1 },
};
constexpr size_t kTriggerDataListLen = sizeof(kTriggerModelDataList) / sizeof(kTriggerModelDataList[0]);
static_assert(kTriggerDataListLen == (size_t) SupportedLocales::Count, "Need trigger data for each supported locale");

const char* kRecognizerModelStr = "enUS_1mb, enUS_500kb, enUS_250kb, \
                                   enUS_Alt_1mb, enUS_Alt_500kb, enUS_Alt_250kb, \
                                   enUK, enAU, frFR, deDE";
const char* kRecognizerModelSensitivityStr = "default,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20";
  
size_t _vectorRecognizerModelTypeIndex = (size_t) SupportedLocales::enUS_500kb;
CONSOLE_VAR_ENUM(size_t, kVectorRecognizerModel, CONSOLE_GROUP_VECTOR, _vectorRecognizerModelTypeIndex, kRecognizerModelStr);

int _vectorTriggerModelSensitivityIndex = 0;
CONSOLE_VAR_ENUM(int, kVectorRecognizerModelSensitivity, CONSOLE_GROUP_VECTOR, _vectorTriggerModelSensitivityIndex,
                 kRecognizerModelSensitivityStr);
  
size_t _alexaRecognizerModelTypeIndex = (size_t) SupportedLocales::enUS_500kb;
CONSOLE_VAR_ENUM(size_t, kAlexaRecognizerModel, CONSOLE_GROUP_ALEXA, _alexaRecognizerModelTypeIndex, kRecognizerModelStr);

int _alexaTriggerModelSensitivityIndex = 0;
CONSOLE_VAR_ENUM(int, kAlexaRecognizerModelSensitivity, CONSOLE_GROUP_ALEXA, _alexaTriggerModelSensitivityIndex,
                 kRecognizerModelSensitivityStr);
  
// HACK
#define CONSOLE_GROUP_ALEXA_PLAYBACK "SpeechRecognizer.AlexaPlayback"
size_t _alexaPlaybackRecognizerModelTypeIndex = (size_t) SupportedLocales::enUS_250kb;
CONSOLE_VAR_ENUM(size_t, kAlexaPlaybackRecognizerModel, CONSOLE_GROUP_ALEXA_PLAYBACK,
                 _alexaPlaybackRecognizerModelTypeIndex, kRecognizerModelStr);

int _alexaPlaybackTriggerModelSensitivityIndex = 0;
CONSOLE_VAR_ENUM(int, kAlexaPlaybackRecognizerModelSensitivity, CONSOLE_GROUP_ALEXA_PLAYBACK,
                 _alexaPlaybackTriggerModelSensitivityIndex, kRecognizerModelSensitivityStr);
  

std::list<Anki::Util::IConsoleFunction> sConsoleFuncs;

#endif // ANKI_DEV_CHEATS
CONSOLE_VAR(bool, kSaveRawMicInput, CONSOLE_GROUP_ALEXA, false);
// 0: don't run; 1: compute power as if _notchDetectorActive; 2: analyze power every tick
CONSOLE_VAR_RANGED(unsigned int, kForceRunNotchDetector, CONSOLE_GROUP_ALEXA, 0, 0, 2);
} // namespace

void SpeechRecognizerSystem::SetupConsoleFuncs()
{
#if ANKI_DEV_CHEATS
  // Update Recognizer Model with kRecognizerModel & kRecognizerModelSensitivity enums
  auto updateVectorRecognizerModel = [this](ConsoleFunctionContextRef context) {
    if (!_victorTrigger) {
      context->channel->WriteLog("'Hey Vector' Trigger is not active");
      return;
    }
    std::string result = UpdateRecognizerHelper(_vectorRecognizerModelTypeIndex, kVectorRecognizerModel,
                                                _vectorTriggerModelSensitivityIndex, kVectorRecognizerModelSensitivity,
                                                *_victorTrigger.get());
    context->channel->WriteLog("UpdateVectorRecognizer %s", result.c_str());
  };
  
  auto updateAlexaRecognizerModel = [this](ConsoleFunctionContextRef context) {
    if (!_alexaTrigger) {
      context->channel->WriteLog("'Alexa' Trigger is not active");
      return;
    }
    std::string result = UpdateRecognizerHelper(_alexaRecognizerModelTypeIndex, kAlexaRecognizerModel,
                                                _alexaTriggerModelSensitivityIndex, kAlexaRecognizerModelSensitivity,
                                                *_alexaTrigger.get());
    context->channel->WriteLog("UpdateAlexaRecognizer %s", result.c_str());
  };
  ////////////////////
  auto updateAlexaPlaybackRecognizerModel = [this](ConsoleFunctionContextRef context) {
    if (!_alexaPlaybackTrigger) {
      context->channel->WriteLog("'Alexa' Playback Trigger is not active");
      return;
    }
    std::string result = UpdateRecognizerHelper(_alexaPlaybackRecognizerModelTypeIndex, kAlexaPlaybackRecognizerModel,
                                                _alexaPlaybackTriggerModelSensitivityIndex, kAlexaPlaybackRecognizerModelSensitivity,
                                                *_alexaPlaybackTrigger.get());
    context->channel->WriteLog("Update Alexa Playback Recognizer %s", result.c_str());
  };

  sConsoleFuncs.emplace_front("Update Vector Recognizer", std::move(updateVectorRecognizerModel),
                              CONSOLE_GROUP_VECTOR, "");
  sConsoleFuncs.emplace_front("Update Alexa Recognizer", std::move(updateAlexaRecognizerModel),
                              CONSOLE_GROUP_ALEXA, "");
  sConsoleFuncs.emplace_front("Update Alexa Playback Recognizer", std::move(updateAlexaPlaybackRecognizerModel),
                              CONSOLE_GROUP_ALEXA_PLAYBACK, "");
  
#endif
  _micDataSystem->GetSpeakerLatency_ms(); // Fix compiler error when ANKI_DEV_CHEATS is not enabled
}
  
std::string SpeechRecognizerSystem::UpdateRecognizerHelper(size_t& inOut_modelIdx, size_t new_modelIdx,
                                                           int& inOut_searchIdx, int new_searchIdx,
                                                           SpeechRecognizerSystem::TriggerContext& trigger)
{
std::string result;
#if ANKI_DEV_CHEATS
  if ((inOut_modelIdx != new_modelIdx) ||
      (inOut_searchIdx != new_searchIdx))
  {
    inOut_modelIdx = new_modelIdx;
    inOut_searchIdx = new_searchIdx;
    const auto& newTypeData = kTriggerModelDataList[new_modelIdx];
    _micDataSystem->SetLocaleDevOnly(newTypeData.locale);
    const int sensitivitySearchFileIdx = (new_searchIdx == 0) ?
                                         newTypeData.searchFileIndex : new_searchIdx;
    
    const bool success = UpdateTriggerForLocale(trigger,
                                                newTypeData.locale,
                                                newTypeData.modelType,
                                                sensitivitySearchFileIdx);
    if (success && (trigger.nextTriggerPaths._netFile.empty())) {
      result = "Recognizer modle or search file was not found, therefore, recognizer was cleared";
    }
    else {
      result = (success ? "success!" : "fail :(");
    }
  }
#endif
  return result;
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
, _notchDetector(std::make_shared<NotchDetector>())
{
  SetupConsoleFuncs();
  
  // TODO: Remove once we use _context to get ALEXA instance
  _context->GetRandom();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeechRecognizerSystem::~SpeechRecognizerSystem()
{
  if( _victorTrigger ) {
    _victorTrigger->recognizer->Stop();
  }
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
  _victorTrigger->micTriggerConfig->Init("hey_vector", dataLoader.GetMicTriggerConfig());
  
  // On Debug builds, check that all the files listed in the trigger config actually exist
#if ANKI_DEVELOPER_CODE
  const auto& triggerDataList = _victorTrigger->micTriggerConfig->GetAllTriggerModelFiles();
  for (const auto& filePath : triggerDataList) {
    const auto& fullFilePath = Util::FileUtils::FullFilePath( {_triggerWordDataDir, filePath} );
    if (Util::FileUtils::FileDoesNotExist(fullFilePath)) {
      LOG_WARNING("SpeechRecognizerSystem.InitVector.MicTriggerConfigFileMissing","%s",fullFilePath.c_str());
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
  
  // wrap callback with another check for whether the input signal contains a notch
  auto wrappedCallback = [callback=std::move(callback), this](const AudioUtil::SpeechRecognizerCallbackInfo& info){
    bool validSignal = true;
    if (_notchDetectorActive || kForceRunNotchDetector) {
      std::lock_guard<std::mutex> lg{_notchMutex};
      validSignal = !_notchDetector->HasNotch();
    }
    if (validSignal) {
      callback(info);
    } else {
      LOG_INFO("SpeechRecognizerSystem.InitAlexaCallback.Notched", "Alexa wake word contained a notch so was ignored");
    }
  };
  
  _alexaComponent = _context->GetAlexa();
  ASSERT_NAMED(_alexaComponent != nullptr, "SpeechRecognizerSystem.InitAlexa._context.GetAlexa.IsNull");
  _alexaTrigger = std::make_unique<TriggerContext>();
  _alexaTrigger->recognizer->Init("");
  _alexaTrigger->recognizer->SetCallback(wrappedCallback);
  _alexaTrigger->recognizer->Start();
  _alexaTrigger->micTriggerConfig->Init("alexa", dataLoader.GetMicTriggerConfig());
  
  // On Debug builds, check that all the files listed in the trigger config actually exist
#if ANKI_DEVELOPER_CODE
  const auto& triggerDataList = _alexaTrigger->micTriggerConfig->GetAllTriggerModelFiles();
  for (const auto& filePath : triggerDataList) {
    const auto& fullFilePath = Util::FileUtils::FullFilePath( {_triggerWordDataDir, filePath} );
    if (Util::FileUtils::FileDoesNotExist(fullFilePath)) {
      LOG_WARNING("SpeechRecognizerSystem.InitAlexa.MicTriggerConfigFileMissing","%s",fullFilePath.c_str());
    }
  }
#endif // ANKI_DEVELOPER_CODE
  
  UpdateTriggerForLocale(locale);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::InitAlexaPlayback(const RobotDataLoader& dataLoader,
                                               TriggerWordDetectedCallback callback)
{
  // This called when Alexa is authorized
  if (_alexaPlaybackTrigger) {
    LOG_WARNING("SpeechRecognizerSystem.InitAlexa", "Alexa Playback Recognizer is already running");
    return;
  }
  
  // HACK: Add Alexa Playback Trigger
  _alexaPlaybackTrigger = std::make_unique<TriggerContext>();
  _alexaPlaybackTrigger->recognizer->Init("");
  _alexaPlaybackTrigger->recognizer->SetCallback(callback);
  _alexaPlaybackTrigger->recognizer->Start();
  _alexaPlaybackTrigger->micTriggerConfig->Init("alexa", dataLoader.GetMicTriggerConfig());
  
  // Set init detector using console vars
  UpdateTriggerForLocale(*_alexaPlaybackTrigger,
                         Util::Locale("en","US"),
                         MicData::MicTriggerConfig::ModelType::size_250kb,
                         -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::DisableAlexa()
{
  if (_alexaTrigger) {
    _alexaTrigger->recognizer->Stop();
    _alexaTrigger.reset();
  }
  if (_alexaPlaybackTrigger) {
    // HACK
    _alexaPlaybackTrigger->recognizer->Stop();
    _alexaPlaybackTrigger.reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::DisableAlexaTemporarily()
{
  if (_alexaTrigger) {
    _alexaTrigger->recognizer->Stop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::ReEnableAlexa()
{
  if (_alexaTrigger) {
    _alexaTrigger->recognizer->Start();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeechRecognizerTHF* SpeechRecognizerSystem::GetAlexaPlaybackRecognizer() {
  if (_alexaPlaybackTrigger) {
    return _alexaPlaybackTrigger->recognizer.get();
  }
  return nullptr;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::ToggleNotchDetector(bool active)
{
  _notchDetectorActive = active;
  // todo: if !active, reset _notchDetector, otherwise it will contain old PSDs in its circular buffer. they get
  // refreshed pretty quickly, so not crucial
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::UpdateRaw(const AudioUtil::AudioSample* audioChunk, unsigned int audioDataLen)
{
  // take 0th channel (back left) and give it to the notch detector
  std::vector<short> singleChannel;
  singleChannel.resize(audioDataLen/MicData::kNumInputChannels);
  for( int i=0, idx=0; i<audioDataLen; i+=MicData::kNumInputChannels, ++idx ) {
    singleChannel[idx] = audioChunk[i];
  }
  
  {
    std::lock_guard<std::mutex> lg{_notchMutex};
    // don't run any ffts if not needed. when _notchDetectorActive is enabled, the notch detector will start computing DFTs
    // and their power and saving that in a circular buffer. when the wake word is used, it averages the recent PSDs and
    // computes some statistics on the average PSD. This means there won't be any data for when the user speaks the first
    // wake word, but that's fine since that won't have a notch anyway
    const bool analyzeSamples = _notchDetectorActive || (kForceRunNotchDetector != 0);
    _notchDetector->AddSamples(singleChannel.data(), singleChannel.size(), analyzeSamples);
    if( kForceRunNotchDetector == 2 ) {
      // run without result. useful for debugging with built-in sine waves
      _notchDetector->HasNotch();
    }
  }
  
  if( ANKI_DEV_CHEATS ) {
    static int pcmfd = -1;
    if( (pcmfd < 0) && kSaveRawMicInput ) {
      const auto path = "/data/data/com.anki.victor/cache/speechRecognizerRaw.pcm";
      pcmfd = open( path, O_CREAT|O_RDWR|O_TRUNC, 0644 );
    }
    
    if( pcmfd >= 0 ) {
      std::vector<short> toSave;
      toSave.resize(audioDataLen/MicData::kNumInputChannels);
      for( int i=0, idx=0; i<audioDataLen; i+=MicData::kNumInputChannels, ++idx ) {
        toSave[idx] = audioChunk[i];
      }
      (void) write( pcmfd, toSave.data(), toSave.size() * sizeof(short) );
      if( !kSaveRawMicInput ) {
        close( pcmfd );
        pcmfd = -1;
      }
    }
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
  if (vadActive && _victorTrigger) {
    _victorTrigger->recognizer->Update(audioData, audioDataLen);
  }

  if (_alexaComponent != nullptr && _alexaTrigger != nullptr) {
    // Update both the alexa SDK and the trigger word at the same time with the same data. This is critical so
    // that their internal sample counters line up
    _alexaComponent->AddMicrophoneSamples(audioData, audioDataLen);
    _alexaTrigger->recognizer->Update(audioData, audioDataLen);

    // NOTE: for the listed reason above, I'm not running the VAD in front of the alexa trigger. If we want to
    // turn that back on, it should be possible, we'd just need to count how many samples were skipped so we
    // could reconcile the sample counters
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeechRecognizerSystem::UpdateTriggerForLocale(const Util::Locale newLocale)
{
  // Set local using defualt locale settings
  bool success = false;
  // We always expect to have victorTrigger
  if( _victorTrigger ) {
    success = UpdateTriggerForLocale(*_victorTrigger.get(), newLocale, MicData::MicTriggerConfig::ModelType::Count, -1);
  }
  if (_alexaTrigger) {
    success &= UpdateTriggerForLocale(*_alexaTrigger.get(), newLocale, MicData::MicTriggerConfig::ModelType::Count, -1);
  }
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeechRecognizerSystem::UpdateTriggerForLocale(TriggerContext& trigger,
                                                    const Util::Locale newLocale,
                                                    const MicData::MicTriggerConfig::ModelType modelType,
                                                    const int searchFileIndex)
{
  std::lock_guard<std::mutex> lock(_triggerModelMutex);
  trigger.nextTriggerPaths = trigger.micTriggerConfig->GetTriggerModelDataPaths(newLocale, modelType, searchFileIndex);
  bool success = false;
  
  if (_victorTrigger && !_victorTrigger->nextTriggerPaths.IsValid()) {
    LOG_WARNING("SpeechRecognizerSystem.UpdateTriggerForLocale.NoPathsFoundForLocale",
                "locale: %s modelType: %d searchFileIndex: %d",
                newLocale.ToString().c_str(), (int) modelType, searchFileIndex);
  }
  
  if (trigger.currentTriggerPaths != trigger.nextTriggerPaths) {
    _isPendingLocaleUpdate = true;
    success = true;
  }
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::ApplyLocaleUpdate()
{
  std::lock_guard<std::mutex> lock(_triggerModelMutex);
  
  TriggerContext* triggers[] = { _victorTrigger.get(), _alexaTrigger.get(), _alexaPlaybackTrigger.get() };
  const size_t kTriggerCount = sizeof(triggers) / sizeof(TriggerContext*);
  for (size_t idx = 0; idx < kTriggerCount; ++idx) {
    TriggerContext* aTrigger = triggers[idx];
    if (aTrigger == nullptr) {
      // It's okay if a trigger doesn't exist, it may not be used.
      continue;
    }
    
    SpeechRecognizerTHF* recognizer = aTrigger->recognizer.get();
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
  
  _isPendingLocaleUpdate = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SpeechRecognizerSystem::TriggerContext
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeechRecognizerSystem::TriggerContext::TriggerContext()
: recognizer(std::make_unique<SpeechRecognizerTHF>())
, micTriggerConfig(std::make_unique<MicData::MicTriggerConfig>())
{
}


} // end namespace Vector
} // end namespace Anki
