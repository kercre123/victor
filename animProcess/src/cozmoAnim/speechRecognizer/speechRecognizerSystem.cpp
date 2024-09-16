/**
 * File: speechRecognizerSystem.cpp
 *
 * Author: Jordan Rivas
 * Created: 10/23/2018
 *
 * Description: Speech Recognizer System handles high-level speech features, such as locale and multiple triggers.
 *
 */

#include "speechRecognizerSystem.h"

#include "audioUtil/speechRecognizer.h"
#include "cozmoAnim/alexa/alexa.h"
#include "cozmoAnim/alexa/media/alexaPlaybackRecognizerComponent.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/robotDataLoader.h"
#include "speechRecognizerPicovoice.h"
#include "cozmoAnim/speechRecognizer/speechRecognizerPryonLite.h"
#include "cozmoAnim/micData/notchDetector.h"
#include "util/console/consoleInterface.h"
#include "util/console/consoleFunction.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/environment/locale.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include <list>
#include <fstream>
#include <sstream>

#include <fcntl.h>
#include <unistd.h>

int chunkNum;

namespace Anki {
namespace Vector {

// VIC-13319 remove
CONSOLE_VAR_EXTERN(bool, kAlexaEnabledInUK);
CONSOLE_VAR_EXTERN(bool, kAlexaEnabledInAU);
  
namespace {
#define LOG_CHANNEL "SpeechRecognizer"

CONSOLE_VAR(bool, kSaveRawMicInput, "SpeechRecognizer.Alexa", false);
// 0: don't run; 1: compute power as if _notchDetectorActive; 2: analyze power every tick
CONSOLE_VAR_RANGED(unsigned int, kForceRunNotchDetector, "SpeechRecognizer.Alexa", 0, 0, 2);
  
CONSOLE_VAR_RANGED(uint, kPlaybackRecognizerSampleCountThreshold, "SpeechRecognizer.AlexaPlayback", 5000, 1000, 10000);

bool AlexaLocaleEnabled(const Util::Locale& locale)
{
  if (locale.GetCountry() == Util::Locale::CountryISO2::US) {
    return true;
  }
  else if (locale.GetCountry() == Util::Locale::CountryISO2::GB) {
    return kAlexaEnabledInUK;
  }
  else if (locale.GetCountry() == Util::Locale::CountryISO2::AU) {
    return kAlexaEnabledInAU;
  }
  else {
    return false;
  }
}

bool AlexaLocaleUsesVad(const Util::Locale& locale)
{
  if ((locale.GetCountry() == Util::Locale::CountryISO2::GB) || (locale.GetCountry() == Util::Locale::CountryISO2::AU)) {
    return false;
  }
  else {
    return true;
  }
}

} // namespace

void SpeechRecognizerSystem::SetupConsoleFuncs()
{
  _micDataSystem->GetSpeakerLatency_ms(); // Fix compiler error when ANKI_DEV_CHEATS is not enabled
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SpeechRecognizerSystem
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeechRecognizerSystem::SpeechRecognizerSystem(const Anim::AnimContext* context,
                                               MicData::MicDataSystem* micDataSystem,
                                               const std::string& triggerWordDataDir)
: _context(context)
, _micDataSystem(micDataSystem)
, _triggerWordDataDir(triggerWordDataDir)
, _notchDetector(std::make_shared<NotchDetector>())
{
  SetupConsoleFuncs();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeechRecognizerSystem::~SpeechRecognizerSystem()
{
  if (_victorTrigger) {
    _victorTrigger->recognizer->Stop();
  }
  if (_alexaTrigger) {
    _alexaTrigger->recognizer->Stop();
  }
  
  // Best way to destroy Alexa recognizer and component
  DisableAlexa();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::InitVector(const Anim::RobotDataLoader& dataLoader,
                                        const Util::Locale& locale,
                                        TriggerWordDetectedCallback callback)
{
  if (_victorTrigger) {
    LOG_WARNING("SpeechRecognizerSystem.InitVector", "Vector Recognizer is already running");
    return;
  }
  
  const bool useVad = true;
  _victorTrigger = std::make_unique<TriggerContextPicovoice>("Vector", useVad);
  _victorTrigger->recognizer->SetCallback(callback);
  
  // Initialize the recognizer
  bool initSuccess = _victorTrigger->recognizer->Init();
  if (!initSuccess) {
    LOG_ERROR("SpeechRecognizerSystem.InitVector", "Failed to initialize Picovoice recognizer");
    return;
  }

  LOG_INFO("SpeechRecognizerSystem.InitVector", "Successfully initialized Picovoice Porcupine!");
  
  //_victorTrigger->recognizer->Start();
  
  // Picovoice doesn't need locale-specific models, so we can skip updating the trigger for locale
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::ToggleNotchDetector(bool active)
{
  _notchDetectorActive = active;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::UpdateNotch(const AudioUtil::AudioSample* audioChunk, unsigned int audioDataLen)
{
  {
    std::lock_guard<std::mutex> lg{_notchMutex};
    const bool analyzeSamples = _notchDetectorActive || (kForceRunNotchDetector != 0);
    _notchDetector->AddSamples(audioChunk, audioDataLen / MicData::kNumInputChannels, analyzeSamples);
    if (kForceRunNotchDetector == 2) {
      _notchDetector->HasNotch();
    }
  }
  
  if (ANKI_DEV_CHEATS) {
    static int pcmfd = -1;
    if ((pcmfd < 0) && kSaveRawMicInput) {
      const auto path = "/data/data/com.anki.victor/cache/speechRecognizerRaw.pcm";
      pcmfd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    }
    
    if (pcmfd >= 0) {
      std::vector<short> toSave;
      toSave.resize(audioDataLen / MicData::kNumInputChannels);
      for (unsigned int i = 0, idx = 0; i < audioDataLen; i += MicData::kNumInputChannels, ++idx) {
        toSave[idx] = audioChunk[i];
      }
      (void)write(pcmfd, toSave.data(), toSave.size() * sizeof(short));
      if (!kSaveRawMicInput) {
        close(pcmfd);
        pcmfd = -1;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::Update(const AudioUtil::AudioSample* audioData, unsigned int audioDataLen, bool vadActive)
{
  // chunkNum++;
  // if (chunkNum >= 3000) {
  //   _victorTrigger->recognizer->DoNew();
  //   chunkNum = 0;
  // }
  if (_victorTrigger && (vadActive || !_victorTrigger->useVad)) {
    _victorTrigger->recognizer->Update(audioData, audioDataLen);
  }
  
  if (_isAlexaActive) {
    if (!_isDisableAlexaPending) {
      _alexaComponent->AddMicrophoneSamples(audioData, audioDataLen);
      _alexaTrigger->recognizer->Update(audioData, audioDataLen);
    }
    else {
      if (_alexaTrigger) {
        _alexaTrigger->recognizer->Stop();
        _alexaTrigger.reset();
      }
      UpdateAlexaActiveState();
      ASSERT_NAMED(!_isAlexaActive, "SpeechRecognizerSystem.DisableAlexa._isAlexaActive.IsTrue");
      _isDisableAlexaPending = false;
      LOG_INFO("SpeechRecognizerSystem.Update", "Alexa mic recognizer has been disabled");
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeechRecognizerSystem::UpdateTriggerForLocale(const Util::Locale& newLocale, RecognizerTypeFlag recognizerFlags)
{
  // Picovoice doesn't require locale updates, so this function can be left empty or return true
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::ActivateAlexa(const Util::Locale& locale, AlexaTriggerWordDetectedCallback callback)
{
  if (_isAlexaActive) {
    LOG_WARNING("SpeechRecognizerSystem.ActivateAlexa",
                "Alexa is already active, must call DisableAlexa() to change state");
    return;
  }
  
  _alexaComponent = _context->GetAlexa();
  
  // Setup Alexa mic recognizer
  InitAlexa(locale, callback);
  
  // Setup Playback Recognizer and operating component
  _alexaPlaybackRecognizerComponent.reset(new AlexaPlaybackRecognizerComponent(_context, *this));
  
  // Second, create the recognizer
  const auto playbackRecognizerCallback = [this](const AudioUtil::SpeechRecognizerCallbackInfo& info)
  {
    _playbackTrigerSampleIdx = _alexaComponent->GetMicrophoneSampleIndex();
  };
  InitAlexaPlayback(locale, playbackRecognizerCallback);

  // Initialize the component now that the recognizer exists
  if (!_alexaPlaybackRecognizerComponent->Init()) {
    _alexaPlaybackRecognizerComponent.reset();
    LOG_ERROR("SpeechRecognizerSystem.ActivateAlexa._alexaPlaybackRecognizerComponent.Init.Failed", "");
  }
  
  UpdateAlexaActiveState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::DisableAlexa()
{
  // Set flag to disable Alexa's recognizer in Update()
  _isDisableAlexaPending = true;
  
  // Destroy component before recognizer so the threads are stopped
  if (_alexaPlaybackRecognizerComponent) {
    _alexaPlaybackRecognizerComponent.reset();
  }
  
  if (_alexaPlaybackTrigger) {
    _alexaPlaybackTrigger->recognizer->Stop();
    _alexaPlaybackTrigger.reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::SetAlexaSpeakingState(bool isSpeaking)
{
  if (_alexaPlaybackRecognizerComponent) {
    _alexaPlaybackRecognizerComponent->SetRecognizerActivate(isSpeaking);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::InitAlexa(const Util::Locale& locale,
                                       const AlexaTriggerWordDetectedCallback callback)
{
  if (_alexaTrigger) {
    LOG_WARNING("SpeechRecognizerSystem.InitAlexa", "Alexa Recognizer is already running");
    return;
  }
  
  // Wrap callback with a check for whether the input signal contains a notch
  const auto wrappedCallback = [callback=std::move(callback), this](const AudioUtil::SpeechRecognizerCallbackInfo& info)
  {
    AudioUtil::SpeechRecognizerIgnoreReason ignoreReason;
    if (_notchDetectorActive || kForceRunNotchDetector) {
      std::lock_guard<std::mutex> lg{_notchMutex};
      ignoreReason.notch = _notchDetector->HasNotch();
    }
    const auto diff = info.endSampleIndex - _playbackTrigerSampleIdx;
    ignoreReason.playback = (diff <= kPlaybackRecognizerSampleCountThreshold);
    
    if (ignoreReason) {
      LOG_INFO("SpeechRecognizerSystem.InitAlexaCallback.Ignored",
               "Alexa wake word contained a notch '%c' or playback recognizer '%c'"
               " samples between playback and user recognizers %llu samples | %llu ms",
               ignoreReason.notch ? 'Y' : 'N', ignoreReason.playback ? 'Y' : 'N', diff, (diff/16));
    }
    callback(info, ignoreReason);
  };
  
  _alexaComponent = _context->GetAlexa();
  const auto dataLoader = _context->GetDataLoader();
  ASSERT_NAMED(_alexaComponent != nullptr, "SpeechRecognizerSystem.InitAlexa._context.GetAlexa.IsNull");
  
  const bool useVad = AlexaLocaleUsesVad(locale);
  _alexaTrigger = std::make_unique<TriggerContextPryon>("Alexa", useVad);
  _alexaTrigger->recognizer->SetCallback(wrappedCallback);
  _alexaTrigger->micTriggerConfig->Init("alexa_pryon", dataLoader->GetMicTriggerConfig());
  _alexaTrigger->recognizer->Start();
  
  // Update the trigger for locale if necessary (Pryon may require locale-specific models)
  UpdateTriggerForLocale(locale, RecognizerTypeFlag::AlexaMic);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::InitAlexaPlayback(const Util::Locale& locale,
                                               TriggerWordDetectedCallback callback)
{
  if (_alexaPlaybackTrigger) {
    LOG_WARNING("SpeechRecognizerSystem.InitAlexaPlayback", "Alexa Playback Recognizer is already running");
    return;
  }
  
  const bool useVad = true;
  
  const auto dataLoader = _context->GetDataLoader();
  _alexaPlaybackTrigger = std::make_unique<TriggerContextPryon>("AlexaPlayback", useVad);
  _alexaPlaybackTrigger->recognizer->SetCallback(callback);
  _alexaPlaybackTrigger->recognizer->SetDetectionThreshold(1); // playback recognizer should be extremely permissive
  _alexaPlaybackTrigger->micTriggerConfig->Init("alexa_pryon", dataLoader->GetMicTriggerConfig());
  
  UpdateTriggerForLocale(locale, RecognizerTypeFlag::AlexaPlayback);
  
  _alexaPlaybackTrigger->recognizer->Start();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::UpdateAlexaActiveState()
{
  _isAlexaActive = (_alexaComponent != nullptr &&
                    _alexaTrigger &&
                    _alexaTrigger->recognizer->IsReady());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class SpeechRecognizerType>
bool SpeechRecognizerSystem::UpdateTriggerForLocale(TriggerContext<SpeechRecognizerType>& trigger,
                                                    const Util::Locale newLocale,
                                                    const MicData::MicTriggerConfig::ModelType modelType,
                                                    const int searchFileIndex)
{
  // For Picovoice, locale updates are not needed
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeechRecognizerSystem::ApplyLocaleUpdate()
{
  // No action needed for Picovoice
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class SpeechRecognizerType>
void SpeechRecognizerSystem::ApplySpeechRecognizerLocaleUpdate(TriggerContext<SpeechRecognizerType>& aTrigger)
{
  // No action needed for Picovoice
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeechRecognizerSystem::UpdateRecognizerModel(TriggerContext<SpeechRecognizerPicovoice>& aTrigger)
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeechRecognizerSystem::UpdateRecognizerModel(TriggerContext<SpeechRecognizerPryonLite>& aTrigger)
{
  bool success = false;
  SpeechRecognizerPryonLite* recognizer = aTrigger.recognizer.get();
  MicData::MicTriggerConfig::TriggerDataPaths& currentTrigPathRef = aTrigger.currentTriggerPaths;
  recognizer->Stop();
  
  if ( currentTrigPathRef.IsValid() ) {
    // Unload & Load
    const std::string netFilePath = currentTrigPathRef.GenerateNetFilePath( _triggerWordDataDir );
    success = recognizer->InitRecognizer( netFilePath, aTrigger.useVad );
    if ( success && (_alexaComponent != nullptr) ) {
      recognizer->SetAlexaMicrophoneOffset( _alexaComponent->GetMicrophoneSampleIndex() );
      recognizer->Start();
    }
  }
  
  return success;
}

} // end namespace Vector
} // end namespace Anki
