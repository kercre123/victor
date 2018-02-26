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

#include "cozmoAnim/animContext.h"
#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/robotDataLoader.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "audioEngine/audioTypeTranslator.h"
#include "audioEngine/plugins/ankiPluginInterface.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"


// Log options
#define LOG_CHANNEL    "TextToSpeech"

// Trace options
#define DEBUG_TEXTTOSPEECH_COMPONENT 0

namespace {
  // TTS audio always plays on robot device
  constexpr Anki::AudioMetaData::GameObjectType kTTSGameObject = Anki::AudioMetaData::GameObjectType::Cozmo_OnDevice;
}

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::TextToSpeechComponent(const AnimContext* context)
: _dispatchQueue(Util::Dispatch::Create("TtSpeechComponent"))
{
  DEV_ASSERT(nullptr != context, "TextToSpeechComponent.InvalidContext");
  DEV_ASSERT(nullptr != context->GetAudioController(), "TextToSpeechComponent.InvalidAudioController");

  _audioController  = context->GetAudioController();

  const Json::Value& tts_config = context->GetDataLoader()->GetTextToSpeechConfig();
  _pvdr = std::make_unique<TextToSpeech::TextToSpeechProvider>(context, tts_config);

} // TextToSpeechComponent()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::~TextToSpeechComponent()
{
  Util::Dispatch::Stop(_dispatchQueue);
  Util::Dispatch::Release(_dispatchQueue);
} // ~TextToSpeechComponent()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result TextToSpeechComponent::CreateSpeech(const TTSID_t ttsID,
                                                                   const std::string& text,
                                                                   const SayTextVoiceStyle style,
                                                                   const float durationScalar)
{
  // Prepare to generate TtS on other thread
  LOG_INFO("TextToSpeechComponent.CreateSpeech", "ttsID %d text '%s' style %hhu durationScalar %f",
           ttsID, Util::HidePersonallyIdentifiableInfo(text.c_str()), style, durationScalar);

  const auto it =_ttsWaveDataMap.emplace(ttsID, TtsBundle());
  if (!it.second) {
    LOG_ERROR("TextToSpeechComponent.CreateSpeech.DispatchAsync", "ttsID %d already in cache", ttsID);
    return RESULT_FAIL_INVALID_PARAMETER;
  }

  // Set initial state
  it.first->second.state = AudioCreationState::Preparing;

  // Dispatch work onto another thread
  Util::Dispatch::Async(_dispatchQueue, [this, ttsID, text, style, durationScalar]
  {
    AudioEngine::StandardWaveDataContainer* audioData = CreateAudioData(text, style, durationScalar);
    {
      std::lock_guard<std::mutex> lock(_lock);
      const auto bundle = GetTtsBundle(ttsID);

      // Check if the ttsBundle is still valid
      if (nullptr == bundle) {
        LOG_INFO("TextToSpeechComponent.CreateSpeech.AsyncDispatch", "No bundle for ttsID %u", ttsID);
        Util::SafeDelete(audioData);
        return;
      }

      // Check if audio was generated for Text to Speech
      if (nullptr == audioData) {
        LOG_ERROR("TextToSpeechComponent.CreateSpeech.DispatchAsync", "No audio data was created for ttsID %u", ttsID);
        bundle->state = AudioCreationState::None;
        return;
      }
      // Put data into cache map
      bundle->state = AudioCreationState::Ready;
      bundle->waveData = audioData;
    }
  });

  return RESULT_OK;
} // CreateSpeech()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::AudioCreationState TextToSpeechComponent::GetOperationState(const TTSID_t ttsID) const
{
  std::lock_guard<std::mutex> lock(_lock);
  const auto * ttsBundle = GetTtsBundle(ttsID);
  if (nullptr == ttsBundle) {
    return AudioCreationState::None;
  }

  return ttsBundle->state;
} // GetOperationState()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Get wwise audio event for a given voice style
//
static AudioMetaData::GameEvent::GenericEvent GetAudioEvent(SayTextVoiceStyle style)
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
//
// Deliver audio data to wwise audio engine
//
bool TextToSpeechComponent::PrepareAudioEngine(const TTSID_t ttsID,
                                               SayTextVoiceStyle style,
                                               float& out_duration_ms)
{
  const auto ttsBundle = GetTtsBundle(ttsID);
  if (nullptr == ttsBundle) {
    LOG_ERROR("TextToSpeechComponent.PrepareAudioEngine", "ttsID %u not found", ttsID);
    return false;
  }

  const auto state = ttsBundle->state;

if (AudioCreationState::Preparing == state) {
    LOG_WARNING("TextToSpeechComponent.PrepareAudioEngine.AudioPreparing", "Audio is not ready");
    ClearOperationData(ttsID);
    return false;
  }

  if (AudioCreationState::None == state) {
    LOG_WARNING("TextToSpeechComponent.PrepareAudioEngine.AudioNone", "Audio is empty");
    ClearOperationData(ttsID);
    return false;
  }

  DEV_ASSERT(AudioCreationState::Ready == ttsBundle->state, "TextToSpeechComponent.PrepareAudioEngine.AudioNotReady");

  if (nullptr == ttsBundle->waveData) {
    LOG_ERROR("TextToSpeechComponent.PrepareAudioEngine", "WaveDataPtr.IsNull");
    return false;
  }

  auto * pluginInterface = _audioController->GetPluginInterface();
  DEV_ASSERT(nullptr != pluginInterface, "TextToSpeechComponent.PrepareAudioEngine.InvalidPluginInterface");

  // Clear previously loaded data
  if (pluginInterface->WavePortalHasAudioDataInfo()) {
    pluginInterface->ClearWavePortalAudioData();
  }

  // Set OUT value
  out_duration_ms = ttsBundle->waveData->ApproximateDuration_ms();

  // Pass ownership of audio data to plugin
  pluginInterface->GiveWavePortalAudioDataOwnership(ttsBundle->waveData);
  ttsBundle->waveData->ReleaseAudioDataOwnership();

  // Tell audio engine to play audio data

  const auto eventId = static_cast<AudioEngine::AudioEventId>(GetAudioEvent(style));
  const auto gameObject = static_cast<AudioEngine::AudioGameObject>(kTTSGameObject);

  _audioController->PostAudioEvent(eventId, gameObject, nullptr);

  // Clear operation from bookkeeping
  ClearOperationData(ttsID);

  return true;
} // PrepareToSay()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::CleanupAudioEngine(const TTSID_t ttsID)
{
  LOG_INFO("TextToSpeechComponent.CleanupAudioEngine", "Clean up ttsID %d", ttsID);

  // Clear previously loaded data
  auto * pluginInterface = _audioController->GetPluginInterface();
  if (pluginInterface->WavePortalHasAudioDataInfo()) {
    pluginInterface->ClearWavePortalAudioData();
  }

  // Clear operation data if needed
  if (kInvalidTTSID != ttsID) {
    ClearOperationData(ttsID);
  }
} // CleanupAudioEngine()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::ClearOperationData(const TTSID_t ttsID)
{
  LOG_INFO("TextToSpeechComponent.ClearOperationData", "Clear ttsID %u", ttsID);

  std::lock_guard<std::mutex> lock(_lock);
  const auto it = _ttsWaveDataMap.find(ttsID);
  if (it != _ttsWaveDataMap.end()) {
    _ttsWaveDataMap.erase(it);
  }
} // ClearOperationData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::ClearAllLoadedAudioData()
{
  LOG_INFO("TextToSpeechComponent.ClearAllLoadedAudioData", "Clear all data");

  std::lock_guard<std::mutex> lock(_lock);
  _ttsWaveDataMap.clear();
} // ClearAllLoadedAudioData()


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
const TextToSpeechComponent::TtsBundle* TextToSpeechComponent::GetTtsBundle(const TTSID_t ttsID) const
{
  const auto iter = _ttsWaveDataMap.find(ttsID);
  if (iter != _ttsWaveDataMap.end()) {
    return &iter->second;
  }

  return nullptr;
} // GetTtsBundle()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::TtsBundle* TextToSpeechComponent::GetTtsBundle(const TTSID_t ttsID)
{
  const auto iter = _ttsWaveDataMap.find(ttsID);
  if (iter != _ttsWaveDataMap.end()) {
    return &iter->second;
  }

  return nullptr;
} // GetTtsBundle()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Determine audio processing state for given voice style
//
static AudioMetaData::SwitchState::Cozmo_Voice_Processing GetAudioProcessingSwitchState(SayTextVoiceStyle style)
{
  using Cozmo_Voice_Processing = AudioMetaData::SwitchState::Cozmo_Voice_Processing;

  switch (style)
  {
    case SayTextVoiceStyle::Unprocessed:
      return Cozmo_Voice_Processing::Unprocessed;
    case SayTextVoiceStyle::CozmoProcessing_Name:
      return Cozmo_Voice_Processing::Name;
    case SayTextVoiceStyle::CozmoProcessing_Name_Question:
      return Cozmo_Voice_Processing::Name;
    case SayTextVoiceStyle::CozmoProcessing_Sentence:
      return Cozmo_Voice_Processing::Sentence;
    default:
      LOG_ERROR("TextToSpeechComponent.GetAudioProcessingSwitchState", "Unexpected style %hhu", style);
      return Cozmo_Voice_Processing::Unprocessed;
  }
}

//
// Set audio processing switch for next utterance
//
void TextToSpeechComponent::SetAudioProcessingStyle(SayTextVoiceStyle style)
{
  const auto switchGroup = AudioMetaData::SwitchState::SwitchGroupType::Cozmo_Voice_Processing;
  const auto switchState = GetAudioProcessingSwitchState(style);

  _audioController->SetSwitchState(
    static_cast<AudioEngine::AudioSwitchGroupId>(switchGroup),
    static_cast<AudioEngine::AudioSwitchStateId>(switchState),
    static_cast<AudioEngine::AudioGameObject>(kTTSGameObject)
  );

}

//
// Set RTPC pitch param for this utterance
//
void TextToSpeechComponent::SetAudioProcessingPitch(float pitchScalar)
{
  // Set Cozmo Says Pitch RTPC Parameter
  const auto paramType = AudioMetaData::GameParameter::ParameterType::External_Process_Pitch;
  const auto paramVal = pitchScalar;

  _audioController->SetParameter(
    static_cast<AudioEngine::AudioParameterId>(paramType),
    static_cast<AudioEngine::AudioRTPCValue>(paramVal),
    static_cast<AudioEngine::AudioGameObject>(kTTSGameObject)
  );
}

//
// Send a TextToSpeechEvent from anim to engine
//
static bool SendEvent(uint8_t ttsID, TextToSpeechState state)
{
  LOG_INFO("TextToSpeechComponent.SendEvent", "ttsID %hhu state %hhu", ttsID, state);
  TextToSpeechEvent evt;
  evt.ttsID = ttsID;
  evt.ttsState = state;
  return AnimProcessMessages::SendAnimToEngine(std::move(evt));
}

//
// Handle incoming TTS request
// Calling thread is blocked until TTS audio has been delivered to audio engine
//
void TextToSpeechComponent::HandleMessage(const RobotInterface::TextToSpeechStart & msg)
{
  using Anki::Util::HidePersonallyIdentifiableInfo;

  // Unpack message fields
  const auto ttsID = msg.ttsID;
  const std::string & text = std::string(msg.text, msg.text_length);
  const auto style = msg.style;
  const auto durationScalar = msg.durationScalar;
  const auto pitchScalar = msg.pitchScalar;

  LOG_DEBUG("TextToSpeechComponent.TextToSpeechStart", "ttsID %d text %s style %hhu durationScalar %f pitchScalar %f",
    ttsID, HidePersonallyIdentifiableInfo(text.c_str()), style, durationScalar, pitchScalar);

  // Send starting event
  SendEvent(ttsID, TextToSpeechState::Starting);

  Result result = CreateSpeech(ttsID, text, style, durationScalar);
  if (RESULT_OK != result) {
    LOG_ERROR("TextToSpeechComponent.TextToSpeechStart", "Unable to create audio data (result %d)", result);
    SendEvent(ttsID, TextToSpeechState::Invalid);
    return;
  }

  // Send running event
  SendEvent(ttsID, TextToSpeechState::Running);

  AudioCreationState state = AudioCreationState::Preparing;
  while (state == AudioCreationState::Preparing) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    state = GetOperationState(ttsID);
  }

  SetAudioProcessingStyle(style);
  SetAudioProcessingPitch(pitchScalar);

  float duration_ms = 0.f;
  PrepareAudioEngine(ttsID, style, duration_ms);

  LOG_INFO("TextToSpeechComponent.TextToSpeechStart", "ttsID %d will play for %.2f ms", ttsID, duration_ms);

  // Send done event
  SendEvent(ttsID, TextToSpeechState::Done);
}

void TextToSpeechComponent::HandleMessage(const RobotInterface::TextToSpeechStop& msg)
{
  const auto ttsID = msg.ttsID;

  LOG_DEBUG("TextToSpeechComponent.HandleMessage.TextToSpeechStop", "ttsID %d", ttsID);

  // TBD: Perform TTS cancel

  // Send done event
  SendEvent(ttsID, TextToSpeechState::Done);
}

} // end namespace Cozmo
} // end namespace Anki
