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

#include "audioEngine/audioCallback.h"
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
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::TextToSpeechComponent(const AnimContext* context)
: _activeTTSID(kInvalidTTSID)
, _dispatchQueue(Util::Dispatch::Create("TtSpeechComponent"))
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
                                           const AudioTtsProcessingStyle style,
                                           const float durationScalar)
{
  // Prepare to generate TTS on other thread
  LOG_INFO("TextToSpeechComponent.CreateSpeech",
           "ttsID %d text '%s' style '%s' durationScalar %f",
           ttsID, Util::HidePersonallyIdentifiableInfo(text.c_str()), EnumToString(style), durationScalar);

  {
    std::lock_guard<std::mutex> lock(_lock);
    const auto it = _ttsWaveDataMap.emplace(ttsID, TtsBundle());
    if (!it.second) {
      LOG_ERROR("TextToSpeechComponent.CreateSpeech.DispatchAsync", "ttsID %d already in cache", ttsID);
      return RESULT_FAIL_INVALID_PARAMETER;
    }

    // Set initial state
    TtsBundle & ttsBundle = it.first->second;
    ttsBundle.state = AudioCreationState::Preparing;
    ttsBundle.style = style;
  }

  // Dispatch work onto another thread
  Util::Dispatch::Async(_dispatchQueue, [this, ttsID, text, durationScalar]
  {
    PushEvent({ttsID, TextToSpeechState::Preparing});
    AudioEngine::StandardWaveDataContainer* audioData = CreateAudioData(text, durationScalar);
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
      PushEvent({ttsID, TextToSpeechState::Prepared});
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
// Deliver audio data to wwise audio engine
//
bool TextToSpeechComponent::PrepareAudioEngine(const TTSID_t ttsID,
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
    return false;
  }

  if (AudioCreationState::None == state) {
    LOG_WARNING("TextToSpeechComponent.PrepareAudioEngine.AudioNone", "Audio is empty");
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

  SetAudioProcessingStyle(ttsBundle->style);

  return true;
} // PrepareAudioEngine()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::CleanupAudioEngine(const TTSID_t ttsID)
{
  LOG_INFO("TextToSpeechComponent.CleanupAudioEngine", "Clean up ttsID %d", ttsID);

  if(ttsID == _activeTTSID){
    StopActiveTTS();
  }

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
  result = _pvdr->CreateAudioData(text, durationScalar, waveData);

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



//
// Set audio processing switch for next utterance
//
void TextToSpeechComponent::SetAudioProcessingStyle(AudioTtsProcessingStyle style)
{
  const auto switchGroup = AudioMetaData::SwitchState::SwitchGroupType::Robot_Vic_External_Processing;
  _audioController->SetSwitchState(
    static_cast<AudioEngine::AudioSwitchGroupId>(switchGroup),
    static_cast<AudioEngine::AudioSwitchStateId>(style),
    static_cast<AudioEngine::AudioGameObject>(kTTSGameObject)
  );
}

//
// Send a TextToSpeechEvent message from anim to engine.
// This is called on main thread for thread-safe access to comms.
//
static bool SendAnimToEngine(uint8_t ttsID, TextToSpeechState state, float expectedDuration = 0.0f)
{
  LOG_DEBUG("TextToSpeechComponent.SendAnimToEngine", "ttsID %hhu state %hhu", ttsID, state);
  TextToSpeechEvent evt;
  evt.ttsID = ttsID;
  evt.ttsState = state;
  evt.expectedDuration_ms = expectedDuration; // Used only on TextToSpeechState::Delivered messages
  return AnimProcessMessages::SendAnimToEngine(std::move(evt));
}

//
// Send audio trigger event for this utterance
//
void TextToSpeechComponent::PostAudioEvent(uint8_t ttsID)
{
  AudioEngine::AudioCallbackContext* audioCallbackContext = nullptr;
  const auto callbackFunc = std::bind(&TextToSpeechComponent::OnUtteranceCompleted, this, ttsID);
  audioCallbackContext = new AudioEngine::AudioCallbackContext();
  // Set callback flags
  audioCallbackContext->SetCallbackFlags( AudioEngine::AudioCallbackFlag::Complete );
  // Execute callbacks synchronously (on main thread)
  audioCallbackContext->SetExecuteAsync( false );
  // Register callbacks for event
  audioCallbackContext->SetEventCallbackFunc ( [ callbackFunc = std::move(callbackFunc) ]
                                              ( const AudioEngine::AudioCallbackContext* thisContext,
                                                const AudioEngine::AudioCallbackInfo& callbackInfo )
                                              {
                                                callbackFunc();
                                              } );

  using AudioEvent = AudioMetaData::GameEvent::GenericEvent;
  const auto eventId = AudioEngine::ToAudioEventId( AudioEvent::Play__Robot_Vic__External_Voice_Text );
  const auto gameObject = static_cast<AudioEngine::AudioGameObject>( kTTSGameObject );
  LOG_DEBUG("TextToSpeechComponent.PostAudioEvent", "ttsID: %hhu gameObject: %u",
            ttsID,
            static_cast<uint32_t>(gameObject));
  AudioEngine::AudioPlayingId playingID = _audioController->PostAudioEvent(eventId, gameObject, audioCallbackContext);
  if(AudioEngine::kInvalidAudioPlayingId != playingID){
    _activeTTSID = ttsID;
    SendAnimToEngine(ttsID, TextToSpeechState::Playing);
  }
  else {
    LOG_DEBUG("TextToSpeechComponent.UtteranceFailedToPlay", "Utterance with ttsID %hhu failed to play", ttsID);
    SendAnimToEngine(ttsID, TextToSpeechState::Invalid);
  }
}

//
// stop the currently playing tts
//
void TextToSpeechComponent::StopActiveTTS()
{
  _audioController->StopAllAudioEvents(static_cast<AudioEngine::AudioGameObject>(kTTSGameObject));
}

//
// Handle a callback from the AudioEngine indicating that the TtS Utterance has finished playing
//
void TextToSpeechComponent::OnUtteranceCompleted(uint8_t ttsID)
{
  _activeTTSID = kInvalidTTSID;

  LOG_DEBUG("TextToSpeechComponent.UtteranceCompleted", "Completion callback received for ttsID %hhu", ttsID);
  SendAnimToEngine(ttsID, TextToSpeechState::Finished);
}

//
// Called on main thread to handle incoming TextToSpeechPrepare
//
void TextToSpeechComponent::HandleMessage(const RobotInterface::TextToSpeechPrepare & msg)
{
  using Anki::Util::HidePersonallyIdentifiableInfo;

  // Unpack message fields
  const auto ttsID = msg.ttsID;
  const std::string text( reinterpret_cast<const char*>(msg.text) );
  const auto style = msg.style;
  const auto durationScalar = msg.durationScalar;

  LOG_DEBUG("TextToSpeechComponent.TextToSpeechPrepare",
            "ttsID %d text %s style %s durationScalar %f",
            ttsID, HidePersonallyIdentifiableInfo(text.c_str()), EnumToString(style), durationScalar);

  // Enqueue request on worker thread
  const Result result = CreateSpeech(ttsID, text, style, durationScalar);
  if (RESULT_OK != result) {
    LOG_ERROR("TextToSpeechComponent.TextToSpeechPrepare", "Unable to create ttsID %d (result %d)", ttsID, result);
    SendAnimToEngine(ttsID, TextToSpeechState::Invalid);
    return;
  }

  // Execution continues in Update() when worker thread posts state change back to main thread

}

//
// Called on main thread to handle incoming TextToSpeechDeliver
void TextToSpeechComponent::HandleMessage(const RobotInterface::TextToSpeechDeliver& msg)
{
  const auto ttsID = msg.ttsID;
  const auto playImmediately = msg.playImmediately;

  LOG_DEBUG("TextToSpeechComponent.TextToSpeechDeliver", "ttsID %d playImmediately %d", ttsID, playImmediately);

  SendAnimToEngine(ttsID, TextToSpeechState::Delivering);

  float duration_ms = 0.f;
  if (!PrepareAudioEngine(ttsID, duration_ms)) {
    LOG_ERROR("TextToSpeechComponent.TextToSpeechDeliver", "Unable to prepare audio engine");
    SendAnimToEngine(ttsID, TextToSpeechState::Invalid);
    ClearOperationData(ttsID);
    return;
  }

  LOG_INFO("TextToSpeechComponent.TextToSpeechDeliver", "ttsID %d will play for %.2f ms", ttsID, duration_ms);

  if (playImmediately) {
    // Tell audio engine to play audio data
    PostAudioEvent(ttsID);
  } else {
    // Keep a record of the ttsID -> AudioEventId for later playback
    _ttsIdSet.emplace(ttsID);
  }

  // Notify engine that tts request is done
  SendAnimToEngine(ttsID, TextToSpeechState::Delivered, duration_ms);

  // Clear operation from bookkeeping
  ClearOperationData(ttsID);
}

//
// Called on main thread to handle incoming TextToSpeechPlay
//
void TextToSpeechComponent::HandleMessage(const RobotInterface::TextToSpeechPlay& msg)
{
  const auto ttsID = msg.ttsID;

  LOG_DEBUG("TextToSpeechComponent.HandleMessage.TextToSpeechPlay", "ttsID %d", ttsID);

  auto it = _ttsIdSet.find(ttsID);
  if(it != _ttsIdSet.end()){
    PostAudioEvent(ttsID);
    _ttsIdSet.erase(ttsID);
  } else {
    LOG_ERROR("TextToSpeechComponent.HandleMessage.TextToSpeechPlay.InvalidRequest",
              "ttsID %d does not correspond to a valid AudioEventId",
              ttsID);
    SendAnimToEngine(ttsID, TextToSpeechState::Invalid);
  }
}

//
// Called on main thread to handle incoming TextToSpeechCancel
//
void TextToSpeechComponent::HandleMessage(const RobotInterface::TextToSpeechCancel& msg)
{
  const auto ttsID = msg.ttsID;

  LOG_DEBUG("TextToSpeechComponent.HandleMessage.TextToSpeechCancel", "ttsID %d", ttsID);

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
void TextToSpeechComponent::OnStatePrepared(const TTSID_t ttsID)
{
  LOG_DEBUG("TextToSpeechComponent.OnStatePrepared", "ttsID %d", ttsID);

  const auto * ttsBundle = GetTtsBundle(ttsID);
  if (nullptr == ttsBundle) {
    LOG_ERROR("TextToSpeechComponent.OnStatePrepared.InvalidBundle", "Can't find bundle for ttsID %d", ttsID);
    SendAnimToEngine(ttsID, TextToSpeechState::Invalid);
    ClearOperationData(ttsID);
    return;
  }

  // Notify engine that tts request has been prepared
  SendAnimToEngine(ttsID, TextToSpeechState::Prepared);

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
      case TextToSpeechState::Prepared:
        OnStatePrepared(ttsID);
        break;
      default:
        //
        // We don't expect any other events from the worker thread.  Transition to Delivering/Delivered are
        // handled by the main thread.
        //
        LOG_ERROR("TextToSpeechComponent.Update.UnexpectedState", "Event ttsID %d unexpected state %hhu",
                  ttsID, ttsState);
        break;
    }
  }
}

void TextToSpeechComponent::SetLocale(const std::string & locale)
{
  //
  // Perform callback on worker thread so locale is changed in sync with TTS processing.
  // Any TTS operations queued before SetLocale() will be processed with old locale.
  // Any TTS operations queued after SetLocale() will be processed with new locale.
  //
  LOG_DEBUG("TextToSpeechComponent.SetLocale", "Set locale to %s", locale.c_str());

  const auto & task = [this, locale = std::string(locale)] {
    DEV_ASSERT(_pvdr != nullptr, "TextToSpeechComponent.SetLocale.InvalidProvider");
    LOG_DEBUG("TextToSpeechComponent.SetLocale", "Setting locale to %s", locale.c_str());
    const Result result = _pvdr->SetLocale(locale);
    if (result != RESULT_OK) {
      LOG_ERROR("TextToSpeechComponent.SetLocale", "Unable to set locale to %s (error %d)", locale.c_str(), result);
    }
  };

  Util::Dispatch::Async(_dispatchQueue, task);

}
} // end namespace Vector
} // end namespace Anki
