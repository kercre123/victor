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
  constexpr Anki::AudioMetaData::GameObjectType kTTSGameObject = Anki::AudioMetaData::GameObjectType::TextToSpeech;
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
void TextToSpeechComponent::PushEvent(const EventPair & evt)
{
  std::lock_guard<std::mutex> lock(_evtq_mutex);
  _evtq.push_back(evt);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TextToSpeechComponent::PopEvent(EventPair & evt)
{
  std::lock_guard<std::mutex> lock(_evtq_mutex);
  if (_evtq.empty()) {
    return false;
  }
  evt = std::move(_evtq.front());
  _evtq.pop_front();
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result TextToSpeechComponent::CreateSpeech(const TTSID_t ttsID,
                                           const std::string& text,
                                           const SayTextVoiceStyle style,
                                           const float durationScalar,
                                           const float pitchScalar)
{
  // Prepare to generate TTS on other thread
  LOG_INFO("TextToSpeechComponent.CreateSpeech", "ttsID %d text '%s' style %hhu durationScalar %f pitchScalar %f",
           ttsID, Util::HidePersonallyIdentifiableInfo(text.c_str()), style, durationScalar, pitchScalar);

  const auto it =_ttsWaveDataMap.emplace(ttsID, TtsBundle());
  if (!it.second) {
    LOG_ERROR("TextToSpeechComponent.CreateSpeech.DispatchAsync", "ttsID %d already in cache", ttsID);
    return RESULT_FAIL_INVALID_PARAMETER;
  }

  // Set initial state
  it.first->second.state = AudioCreationState::Preparing;
  it.first->second.style = style;
  it.first->second.pitchScalar = pitchScalar;

  // Dispatch work onto another thread
  Util::Dispatch::Async(_dispatchQueue, [this, ttsID, text, style, durationScalar]
  {
    PushEvent({ttsID, TextToSpeechState::Preparing});
    AudioEngine::StandardWaveDataContainer* audioData = CreateAudioData(text, style, durationScalar);
    {
      std::lock_guard<std::mutex> lock(_lock);
      const auto bundle = GetTtsBundle(ttsID);

      // Check if the ttsBundle is still valid
      if (nullptr == bundle) {
        LOG_INFO("TextToSpeechComponent.CreateSpeech.AsyncDispatch", "No bundle for ttsID %u", ttsID);
        Util::SafeDelete(audioData);
        PushEvent({ttsID, TextToSpeechState::Invalid});
        return;
      }

      // Check if audio was generated for Text to Speech
      if (nullptr == audioData) {
        LOG_ERROR("TextToSpeechComponent.CreateSpeech.DispatchAsync", "No audio data was created for ttsID %u", ttsID);
        bundle->state = AudioCreationState::None;
        PushEvent({ttsID, TextToSpeechState::Invalid});
        return;
      }
      // Put data into cache map
      bundle->state = AudioCreationState::Ready;
      bundle->waveData = audioData;
      PushEvent({ttsID, TextToSpeechState::Ready});
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
    case SayTextVoiceStyle::CozmoProcessing_Sentence:
    case SayTextVoiceStyle::CozmoProcessing_Name:
      return AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vo__External_Cozmo_Processing;
    case SayTextVoiceStyle::CozmoProcessing_Name_Question:
      return AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vo__External_Cozmo_Processing_Question;
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

  _audioController->PostAudioEvent(eventId, gameObject);

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
// Send a TextToSpeechEvent message from anim to engine.
// This is called on main thread for thread-safe access to comms.
//
static bool SendAnimToEngine(uint8_t ttsID, TextToSpeechState state)
{
  LOG_DEBUG("TextToSpeechComponent.SendAnimToEngine", "ttsID %hhu state %hhu", ttsID, state);
  TextToSpeechEvent evt;
  evt.ttsID = ttsID;
  evt.ttsState = state;
  return AnimProcessMessages::SendAnimToEngine(std::move(evt));
}

//
// Called on main thread to handle incoming TextToSpeechStart
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

  // Enqueue request on worker thread
  const Result result = CreateSpeech(ttsID, text, style, durationScalar, pitchScalar);
  if (RESULT_OK != result) {
    LOG_ERROR("TextToSpeechComponent.TextToSpeechStart", "Unable to create ttsID %d (result %d)", ttsID, result);
    SendAnimToEngine(ttsID, TextToSpeechState::Invalid);
    return;
  }

  // Execution continues in Update() when worker thread posts state change back to main thread

}

//
// Called on main thread to handle incoming TextToSpeechStop
//
void TextToSpeechComponent::HandleMessage(const RobotInterface::TextToSpeechStop& msg)
{
  const auto ttsID = msg.ttsID;

  LOG_DEBUG("TextToSpeechComponent.HandleMessage.TextToSpeechStop", "ttsID %d", ttsID);

  // TBD: Cancel animation
  CleanupAudioEngine(ttsID);

  // Notify engine that request is now invalid
  SendAnimToEngine(ttsID, TextToSpeechState::Invalid);
}

//
// Called by Update() in response to event from worker thread
//
void TextToSpeechComponent::OnStateInvalid(const TTSID_t ttsID)
{
  LOG_DEBUG("TextToSpeechComponent.OnStateInvalid", "ttsID %d", ttsID);

  // Notify engine that tts request has failed
  SendAnimToEngine(ttsID, TextToSpeechState::Invalid);

  // Clean up request state
  ClearOperationData(ttsID);
}

//
// Called by Update() in response to event from worker thread
//
void TextToSpeechComponent::OnStatePreparing(const TTSID_t ttsID)
{
  LOG_DEBUG("TextToSpeechComponent.OnStatePreparing", "ttsID %d", ttsID);

  // Notify engine that tts request is being prepared.
  // This is currently a no-op for the engine side but
  // it seems like the right thing to do.
  SendAnimToEngine(ttsID, TextToSpeechState::Preparing);
}

//
// Called by Update() in response to event from worker thread
//
void TextToSpeechComponent::OnStateReady(const TTSID_t ttsID)
{
  LOG_DEBUG("TextToSpeechComponent.OnStateReady", "ttsID %d", ttsID);

  const auto * ttsBundle = GetTtsBundle(ttsID);
  if (nullptr == ttsBundle) {
    LOG_ERROR("TextToSpeechComponent.OnStateReady.InvalidBundle", "Can't find bundle for ttsID %d", ttsID);
    SendAnimToEngine(ttsID, TextToSpeechState::Invalid);
    ClearOperationData(ttsID);
    return;
  }

  // Notify engine that tts request is ready to play
  SendAnimToEngine(ttsID, TextToSpeechState::Ready);

  //
  // TBD: Start streaming animation.
  // For now, just start playing audio.
  //

  // Notify engine that tts request is now playing
  SendAnimToEngine(ttsID, TextToSpeechState::Playing);

  SetAudioProcessingStyle(ttsBundle->style);
  SetAudioProcessingPitch(ttsBundle->pitchScalar);

  float duration_ms = 0.f;
  PrepareAudioEngine(ttsID, ttsBundle->style, duration_ms);

  LOG_INFO("TextToSpeechComponent.TextToSpeechStart", "ttsID %d will play for %.2f ms", ttsID, duration_ms);

  // Notify engine that tts request is done
  SendAnimToEngine(ttsID, TextToSpeechState::Done);

  // Clean up request state
  ClearOperationData(ttsID);
}

//
// Called by main thread (once per tick) to handle events posted by worker thread
//
void TextToSpeechComponent::Update()
{
  EventPair evt;

  // Process events posted by worker thread
  while (PopEvent(evt)) {
    const auto ttsID = evt.first;
    const auto ttsState = evt.second;

    LOG_DEBUG("TextToSpeechComponent.Update", "Event ttsID %d state %hhu", ttsID, ttsState);

    switch (ttsState) {
      case TextToSpeechState::Invalid:
        OnStateInvalid(ttsID);
        break;
      case TextToSpeechState::Preparing:
        OnStatePreparing(ttsID);
        break;
      case TextToSpeechState::Ready:
        OnStateReady(ttsID);
        break;
      default:
        //
        // We don't expect any other events from the worker thread.  Transition to Playing and Done are
        // handled by the main thread.
        //
        LOG_ERROR("TextToSpeechComponent.Update.UnexpectedState", "Event ttsID %d unexpected state %hhu",
                  ttsID, ttsState);
        break;
    }
  }
}
} // end namespace Cozmo
} // end namespace Anki
