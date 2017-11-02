/**
 * File: textToSpeechComponent.cpp
 *
 * Author: Molly Jameson
 * Created: 3/21/16
 *
 * Overhaul: Andrew Stein / Jordan Rivas, 5/02/16
 *
 * Description: Component wrapper to generate, cache and use wave data from a given string and style.
 * This class provides a platform-independent interface to separate engine & audio libraries from
 * details of a specific text-to-speech implementation.
 *
 * Copyright: Anki, Inc. 2016
 *
 */

#include "textToSpeechComponent.h"
#include "textToSpeechProvider.h"

#include "cozmoAnim/cozmoAnimContext.h"
#include "cozmoAnim/robotDataLoader.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "audioEngine/audioEngineController.h"
#include "audioEngine/multiplexer/audioMultiplexer.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"


// Log options
#define LOG_CHANNEL    "TextToSpeech"
#define LOG_ERROR      PRINT_NAMED_ERROR
#define LOG_WARNING    PRINT_NAMED_WARNING
#define LOG_INFO(...)  PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)
#define LOG_DEBUG(...) PRINT_CH_DEBUG(LOG_CHANNEL, ##__VA_ARGS__)

// Trace options
#define DEBUG_TEXTTOSPEECH_COMPONENT 0

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::TextToSpeechComponent(const CozmoAnimContext* context)
: _dispatchQueue(Util::Dispatch::Create("TtSpeechComponent"))
{
  const Json::Value& tts_config = context->GetDataLoader()->GetTextToSpeechConfig();
  _pvdr = std::make_unique<TextToSpeech::TextToSpeechProvider>(context, tts_config);

//  if (nullptr != context && nullptr != context->GetAudioMultiplexer()) {
//    _audioController = context->GetAudioMultiplexer()->GetAudioController();
//  }
} // TextToSpeechComponent()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::~TextToSpeechComponent()
{
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
  LOG_INFO("TextToSpeechComponent.CreateSpeech", "Text '%s' Style: %d Duration: %f OperationId: %u",
           Util::HidePersonallyIdentifiableInfo(text.c_str()), // could be a name!
           (int) style, durationScalar, opId);
  
  const auto it =_ttsWaveDataMap.emplace(opId, TtsBundle());
  if (!it.second) {
    LOG_ERROR("TextToSpeechComponent.CreateSpeech.DispatchAsync", "OperationId %d already in cache", opId);
    return kInvalidOperationId;
  }

  // Set initial state
  it.first->second.state = AudioCreationState::Preparing;

  // Dispatch work onto another thread
  Util::Dispatch::Async(_dispatchQueue, [this, text, style, durationScalar, opId]
  {
    AudioEngine::StandardWaveDataContainer* audioData = CreateAudioData(text, style, durationScalar);
    {
      std::lock_guard<std::mutex> lock(_lock);
      const auto bundle = GetTtsBundle(opId);
      
      // Check if the ttsBundle is still valid
      if (nullptr == bundle) {
        LOG_INFO("TextToSpeechComponent.CreateSpeech.AsyncDispatch", "OperationId: %u for Tts Bundle NOT found", opId);
        Util::SafeDelete(audioData);
        return;
      }
      
      // Check if audio was generated for Text to Speech
      if (nullptr == audioData) {
        LOG_ERROR("TextToSpeechComponent.CreateSpeech.DispatchAsync", "No Audio data was created");
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
    LOG_ERROR("TextToSpeechComponent.PrepareAudioEngine", "OperationId: %u Not Found", operationId);
    return false;
  }
  
  LOG_INFO("TextToSpeechComponent.PrepareAudioEngine", "OperationId: %u", operationId);
  
  DEV_ASSERT(AudioCreationState::Ready == ttsBundle->state,
             "TextToSpeechComponent.PrepareAudioEngine.ttsBundle.state.NotReady");
  
  if (nullptr == ttsBundle->waveData) {
    LOG_ERROR("TextToSpeechComponent.PrepareAudioEngine", "WaveDataPtr.IsNull");
    return false;
  }
  
//  using namespace AudioEngine::PlugIns;
//  DEV_ASSERT(nullptr != _audioController, "TextToSpeechComponent.PrepareAudioEngine.NullAudioController");
  
//  AnkiPluginInterface* pluginInterface = _audioController->GetPluginInterface();
//  DEV_ASSERT(pluginInterface != nullptr, "TextToSpeechComponent.PrepareAudioEngine.NullAudioControllerPluginInterface");
  
  // Clear previously loaded data
//  if (pluginInterface->WavePortalHasAudioDataInfo()) {
//    pluginInterface->ClearWavePortalAudioData();
//  }

  // Set OUT value
  out_duration_ms = ttsBundle->waveData->ApproximateDuration_ms();
//  // Pass ownership of audio data to plugin
//  pluginInterface->GiveWavePortalAudioDataOwnership(ttsBundle->waveData);
  // Release ownership of audio data and clear operation form bookkeeping
  ttsBundle->waveData->ReleaseAudioDataOwnership();
  ClearOperationData(operationId);
  
  return true;
} // PrepareToSay()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::CleanupAudioEngine(const OperationId operationId)
{
  LOG_INFO("TextToSpeechComponent.CleanupAudioEngine", "");
  
//  using namespace AudioEngine::PlugIns;
//  DEV_ASSERT(nullptr != _audioController, "TextToSpeechComponent.CleanupAudioEngine.NullAudioController");
//  AnkiPluginInterface* pluginInterface = _audioController->GetPluginInterface();
//  DEV_ASSERT(pluginInterface != nullptr, "TextToSpeechComponent.CleanupAudioEngine.NullAudioControllerPluginInterface");
//  
//  // Clear previously loaded data
//  if (pluginInterface->WavePortalHasAudioDataInfo()) {
//    pluginInterface->ClearWavePortalAudioData();
//  }
  
  // Clear operation data if needed
  if (TextToSpeechComponent::kInvalidOperationId != operationId) {
    ClearOperationData(operationId);
  }
} // CompltedSpeech()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::ClearOperationData(const OperationId operationId)
{
  LOG_INFO("TextToSpeechComponent.ClearOperationData", "OperationId: %u", operationId);
  
  std::lock_guard<std::mutex> lock(_lock);
  const auto it = _ttsWaveDataMap.find(operationId);
  if (it != _ttsWaveDataMap.end()) {
    _ttsWaveDataMap.erase(it);
  }
} // ClearOperationData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::ClearAllLoadedAudioData()
{
  LOG_INFO("TextToSpeechComponent.ClearAllLoadedAudioData", "");
  
  std::lock_guard<std::mutex> lock(_lock);
  _ttsWaveDataMap.clear();
} // ClearAllLoadedAudioData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioMetaData::GameEvent::GenericEvent TextToSpeechComponent::GetAudioEvent(SayTextVoiceStyle style) const
{
  switch (style) {
    case SayTextVoiceStyle::Unprocessed:
      return AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vo__External_Unprocessed;
      break;
      
    case SayTextVoiceStyle::CozmoProcessing_Sentence:
    case SayTextVoiceStyle::CozmoProcessing_Name:
      return AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vo__External_Cozmo_Processing;
      break;
      
    case SayTextVoiceStyle::CozmoProcessing_Name_Question:
      return AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vo__External_Cozmo_Processing_Question;
      break;

    case SayTextVoiceStyle::Count:
      LOG_ERROR("TextToSpeechComponent.GetAudioEvent", "Invalid SayTextStyle Count");
      break;
  }
  return AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vo__External_Unprocessed;
} // GetAudioEvent()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioEngine::StandardWaveDataContainer* TextToSpeechComponent::CreateAudioData(const std::string& text,
                                                                               SayTextVoiceStyle style,
                                                                               float durationScalar)
{
  using namespace Util::Time;
  using namespace TextToSpeech;
  double time_ms = 0.0;
  
  if (DEBUG_TEXTTOSPEECH_COMPONENT) {
    time_ms = UniversalTime::GetCurrentTimeInMilliseconds();
    LOG_INFO("TextToSpeechComponent.CreateAudioData", "Start - text to wave: %s - duration scalar: %f",
             text.c_str(), durationScalar);
  }

  Result result = RESULT_OK;
  TextToSpeechProviderData waveData;

  if (SayTextVoiceStyle::CozmoProcessing_Name_Question == style) {
    result = _pvdr->CreateAudioData(text + "?", durationScalar, waveData);
  } else {
    result = _pvdr->CreateAudioData(text, durationScalar, waveData);
  }
  
  if (DEBUG_TEXTTOSPEECH_COMPONENT) {
    LOG_INFO("TextToSpeechComponent.CreateAudioData", "finish text to wave - time_ms: %f",
             UniversalTime::GetCurrentTimeInMilliseconds() - time_ms);
  }
  
  if (RESULT_OK != result) {
    LOG_ERROR("TextToSpeechComponent.CreateAudioData", "Unable to create audio data (error %d)", result);
    return nullptr;
  }
  
  const int sample_rate = waveData.GetSampleRate();
  const int num_channels = waveData.GetNumChannels();
  const size_t num_samples = waveData.GetNumSamples();
  const short * samples = waveData.GetSamples();
  
  if (num_samples == 0) {
    // Not necessarily an error
    return nullptr;
  }
  
  // Create Standard Wave from audio data
  using namespace AudioEngine;
  
  StandardWaveDataContainer* data = new StandardWaveDataContainer(sample_rate, num_channels, num_samples);
  
  // Convert waveData format into StandardWaveDataContainer's format
  const float kOneOverSHRT_MAX = 1.0f / float(SHRT_MAX);
  for (size_t sampleIdx = 0; sampleIdx < data->bufferSize; ++sampleIdx) {
    data->audioBuffer[sampleIdx] = samples[sampleIdx] * kOneOverSHRT_MAX;
  }
  
  if (DEBUG_TEXTTOSPEECH_COMPONENT) {
    LOG_INFO("TextToSpeechComponent.CreateAudioData", "Finish convert samples - time_ms: %f",
             UniversalTime::GetCurrentTimeInMilliseconds() - time_ms);
  }
  
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
