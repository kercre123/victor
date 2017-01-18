/**
 * File: textToSpeechComponent.cpp
 *
 * Author: Molly Jameson
 * Created: 3/21/16
 *
 * Overhaul: Andrew Stein / Jordan Rivas, 5/02/16
 *
 * Description: Flite wrapper to generate, cache and use wave data from a given string and style.
 *
 * Copyright: Anki, inc. 2016
 *
 */


extern "C" {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include "flite.h"
#pragma GCC diagnostic pop
  cst_voice* register_cmu_us_rms(const char* voxdir);
  void unregister_cmu_us_rms(cst_voice* vox);
}

#include "anki/cozmo/basestation/textToSpeech/textToSpeechComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioControllerPluginInterface.h"
#include "anki/cozmo/basestation/audio/audioServer.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/time/universalTime.h"

#define DEBUG_TEXTTOSPEECH_COMPONENT 0


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::TextToSpeechComponent(const CozmoContext* context)
: _dispatchQueue(Util::Dispatch::Create("TtSpeechComponent"))
{
  flite_init();
  
  _voice = register_cmu_us_rms(nullptr);
  
  if(nullptr != context && nullptr != context->GetAudioServer()) {
    _audioController = context->GetAudioServer()->GetAudioController();
  }
} // TextToSpeechComponent()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::~TextToSpeechComponent()
{
  // There is no tear-down for flite =(
  unregister_cmu_us_rms(_voice);
  Util::Dispatch::Stop(_dispatchQueue);
  Util::Dispatch::Release(_dispatchQueue);
} // ~TextToSpeechComponent()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::OperationId TextToSpeechComponent::CreateSpeech(const std::string& text,
                                                                       const SayTextVoiceStyle style,
                                                                       const float durationScalar)
{
  // Prepare to generate TtS on other thread
  OperationId opId = GetNextOperationId();
  PRINT_CH_INFO(Audio::AudioController::kAudioLogChannelName,
                "TextToSpeechComponent.CreateSpeech",
                "Text '%s' Style: %s Duration: %f OperationId: %u",
                Util::HidePersonallyIdentifiableInfo(text.c_str()), // could be a name!
                EnumToString(style), durationScalar, opId);
  
  const auto it =_ttsWaveDataMap.emplace(opId, TtsBundle());
  if (!it.second) {
    PRINT_NAMED_ERROR("TextToSpeechComponent.CreateSpeech.DispatchAysnc", "OperationId %d already in cache", opId);
    return kInvalidOperationId;
  }
  // Set inital state
  it.first->second.state = AudioCreationState::Preparing;
  // Dispatch work onto another thread
  Util::Dispatch::Async(_dispatchQueue, [this, text, durationScalar, opId]
  {
    Audio::StandardWaveDataContainer* audioData = CreateAudioData(text, durationScalar);
    {
      std::lock_guard<std::mutex> lock(_lock);
      const auto bundle = GetTtsBundle(opId);
      
      // Check if the ttsBundle is still valid
      if (nullptr == bundle) {
        PRINT_CH_INFO(Audio::AudioController::kAudioLogChannelName,
                      "TextToSpeechComponent.CreateSpeech.AsyncDispatch",
                      "OperationId: %u for Tts Bundle NOT found", opId);
        Util::SafeDelete(audioData);
        return;
      }
      
      // Check if audio was generated for Text to Speech
      if (nullptr == audioData) {
        PRINT_NAMED_ERROR("TextToSpeechComponent.CreateSpeech.DispatchAysnc", "No Audio data was created");
        bundle->state = AudioCreationState::None;
        return;
      }
      // Put data into cache map
      bundle->state = AudioCreationState::Ready;
      bundle->waveData = audioData;
    }
  });
 
  return opId;
} // CreateSpeech()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::AudioCreationState TextToSpeechComponent::GetOperationState(const OperationId operationId) const
{
  std::lock_guard<std::mutex> lock(_lock);
  const auto ttsBundle = GetTtsBundle(operationId);
  if (nullptr == ttsBundle) {
    return AudioCreationState::None;
  }
  
  return ttsBundle->state;
} // GetOperationState()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TextToSpeechComponent::PrepareAudioEngine(const OperationId operationId,
                                               float& out_duration_ms)
{
  const auto ttsBundle = GetTtsBundle(operationId);
  if (nullptr == ttsBundle) {
    PRINT_NAMED_ERROR("TextToSpeechComponent.PrepareAudioEngine", "OperationId: %u Not Found", operationId);
    return false;
  }
  
  PRINT_CH_INFO(Audio::AudioController::kAudioLogChannelName,
                "TextToSpeechComponent.PrepareAudioEngine",
                "OperationId: %u",
                operationId);
  
  DEV_ASSERT(AudioCreationState::Ready == ttsBundle->state,
             "TextToSpeechComponent.PrepareAudioEngine.ttsBundle.state.NotReady");
  
  if (nullptr == ttsBundle->waveData) {
    PRINT_NAMED_ERROR("TextToSpeechComponent.PrepareAudioEngine", "WaveDataPtr.IsNull");
    return false;
  }
  
  using namespace Audio;
  DEV_ASSERT(nullptr != _audioController, "TextToSpeechComponent.PrepareAudioEngine.NullAudioController");
  AudioControllerPluginInterface* pluginInterface = _audioController->GetPluginInterface();
  DEV_ASSERT(pluginInterface != nullptr, "TextToSpeechComponent.PrepareAudioEngine.NullAudioControllerPluginInterface");
  
  // Clear previously loaded data
  if (pluginInterface->WavePortalHasAudioDataInfo()) {
    pluginInterface->ClearWavePortalAudioData();
  }

  // Set OUT value
  out_duration_ms = ttsBundle->waveData->ApproximateDuration_ms();
  // Pass ownership of audio data to plugin
  pluginInterface->GiveWavePortalAudioDataOwnership(ttsBundle->waveData);
  // Release ownership of audio data and clear operation form bookkeeping
  ttsBundle->waveData->ReleaseAudioDataOwnership();
  ClearOperationData(operationId);
  
  return true;
} // PrepareToSay()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::CleanupAudioEngine(const OperationId operationId)
{
  PRINT_CH_INFO(Audio::AudioController::kAudioLogChannelName,
                "TextToSpeechComponent.CleanupAudioEngine", "");
  
  using namespace Audio;
  DEV_ASSERT(nullptr != _audioController, "TextToSpeechComponent.CleanupAudioEngine.NullAudioController");
  AudioControllerPluginInterface* pluginInterface = _audioController->GetPluginInterface();
  DEV_ASSERT(pluginInterface != nullptr, "TextToSpeechComponent.CleanupAudioEngine.NullAudioControllerPluginInterface");
  
  // Clear previously loaded data
  if (pluginInterface->WavePortalHasAudioDataInfo()) {
    pluginInterface->ClearWavePortalAudioData();
  }
  
  // Clear operation data if needed
  if (TextToSpeechComponent::kInvalidOperationId != operationId) {
    ClearOperationData(operationId);
  }
} // CompltedSpeech()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::ClearOperationData(const OperationId operationId)
{
  PRINT_CH_INFO(Audio::AudioController::kAudioLogChannelName,
                "TextToSpeechComponent.ClearOperationData",
                "OperationId: %u",
                operationId);
  
  std::lock_guard<std::mutex> lock(_lock);
  const auto it = _ttsWaveDataMap.find(operationId);
  if (it != _ttsWaveDataMap.end()) {
    _ttsWaveDataMap.erase(it);
  }
} // ClearOperationData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::ClearAllLoadedAudioData()
{
  PRINT_CH_INFO(Audio::AudioController::kAudioLogChannelName,
                "TextToSpeechComponent.ClearAllLoadedAudioData", "");
  
  std::lock_guard<std::mutex> lock(_lock);
  _ttsWaveDataMap.clear();
} // ClearAllLoadedAudioData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Audio::GameEvent::GenericEvent TextToSpeechComponent::GetAudioEvent(SayTextVoiceStyle style) const
{
  switch (style) {
    case SayTextVoiceStyle::Unprocessed:
      return Audio::GameEvent::GenericEvent::Play__Robot_Vo__External_Unprocessed;
      break;
      
    case SayTextVoiceStyle::CozmoProcessing_Sentence:
    case SayTextVoiceStyle::CozmoProcessing_Name:
      return Audio::GameEvent::GenericEvent::Play__Robot_Vo__External_Cozmo_Processing;
      break;
      
    case SayTextVoiceStyle::Count:
      PRINT_NAMED_ERROR("TextToSpeechComponent.GetAudioEvent", "Invalid SayTextStyle Count");
      break;
  }
  return Audio::GameEvent::GenericEvent::Play__Robot_Vo__External_Unprocessed;
} // GetAudioEvent()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Audio::StandardWaveDataContainer* TextToSpeechComponent::CreateAudioData(const std::string& text,
                                                                         const float durationScalar)
{
  using namespace Util::Time;
  double time_ms = 0.0;
  if (DEBUG_TEXTTOSPEECH_COMPONENT) {
    time_ms = UniversalTime::GetCurrentTimeInMilliseconds();
    PRINT_CH_INFO(Audio::AudioController::kAudioLogChannelName,
                  "TextToSpeechComponent.CreateAudioData",
                  "Start - text to wave: %s - duration scalar: %f", text.c_str(), durationScalar);
  }
  
  // Add Duration Stretch feature to voice
  // If durationScalar is too small, the flite code crashes; so we clamp it here
  feat_set_float(_voice->features, "duration_stretch", std::max(durationScalar, 0.05f));
  // Generate wave data
  cst_wave* waveData = flite_text_to_wave(text.c_str(), _voice);
  
  if (DEBUG_TEXTTOSPEECH_COMPONENT) {
    PRINT_CH_INFO(Audio::AudioController::kAudioLogChannelName,
                  "TextToSpeechComponent.CreateAudioData",
                  "finish text to wave - time_ms: %f", UniversalTime::GetCurrentTimeInMilliseconds() - time_ms);
  }
  
  if (waveData->num_samples == 0) {
    delete_wave(waveData);
    return nullptr;
  }
  
  // Create Standard Wave
  Audio::StandardWaveDataContainer* data = new Audio::StandardWaveDataContainer(waveData->sample_rate,
                                                                                waveData->num_channels,
                                                                                (size_t)waveData->num_samples);

  // Convert waveData format into StandardWaveDataContainer's format
  const float kOneOverSHRT_MAX = 1.0f / float(SHRT_MAX);
  for (size_t sampleIdx = 0; sampleIdx < data->bufferSize; ++sampleIdx) {
    data->audioBuffer[sampleIdx] = waveData->samples[sampleIdx] * kOneOverSHRT_MAX;
  }
  
  if (DEBUG_TEXTTOSPEECH_COMPONENT) {
    PRINT_CH_INFO(Audio::AudioController::kAudioLogChannelName,
                  "TextToSpeechComponent.CreateAudioData",
                  "Finish convert samples - time_ms: %f", UniversalTime::GetCurrentTimeInMilliseconds() - time_ms);
  }
  
  delete_wave(waveData);
  
  return data;
} // CreateAudioData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const TextToSpeechComponent::TtsBundle* TextToSpeechComponent::GetTtsBundle(const OperationId operationId) const
{
  const auto iter = _ttsWaveDataMap.find(operationId);
  if (iter != _ttsWaveDataMap.end()) {
    return &iter->second;
  }
  
  return nullptr;
} // GetTtsBundle()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::TtsBundle* TextToSpeechComponent::GetTtsBundle(const OperationId operationId)
{
  const auto iter = _ttsWaveDataMap.find(operationId);
  if (iter != _ttsWaveDataMap.end()) {
    return &iter->second;
  }
  
  return nullptr;
} // GetTtsBundle()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::OperationId TextToSpeechComponent::GetNextOperationId()
{
  if (++_prevOperationId == kInvalidOperationId) {
    ++_prevOperationId;
  }
  return _prevOperationId;
} // GetNextOperationId()

} // end namespace Cozmo
} // end namespace Anki

