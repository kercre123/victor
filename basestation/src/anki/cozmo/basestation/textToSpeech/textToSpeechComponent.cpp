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
#include "clad/audio/audioParameterTypes.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/fileUtils/fileUtils.h"
#include "util/hashing/hashing.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/time/universalTime.h"

#define DEBUG_TEXTTOSPEECH_COMPONENT 0


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::TextToSpeechComponent(const CozmoContext* context)
: _dispatchQueue( Util::Dispatch::Create("TtSpeechComponent") )
{
  flite_init();
  
  _voice = register_cmu_us_rms( nullptr );
  
  // Add Duration Stretch feature to voice
  // Create text with 2x length, Wwise will time stretch them to the desired length
  // feat_set_float( _voice->features, "duration_stretch", 2.0 );
  // ^^^^ Just kiding we don't know what we want ^^^^
  
  
  if( nullptr != context ) {
    if( nullptr != context->GetAudioServer() ) {
      _audioController = context->GetAudioServer()->GetAudioController();
    }
  }

} // TextToSpeechComponent()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::~TextToSpeechComponent()
{
  // There is no tear-down for flite =(
  unregister_cmu_us_rms( _voice );
  Util::Dispatch::Stop( _dispatchQueue );
  Util::Dispatch::Release( _dispatchQueue );
} // ~TextToSpeechComponent()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::SpeechState TextToSpeechComponent::CreateSpeech(const std::string& text, SayTextStyle style)
{
  // Setup meta data before async creation of audio data
  // Check if bundle already exist for text
  SpeechState state;
  {
    std::lock_guard<std::mutex> lock( _lock );
    TextPhraseBundle* phraseBundle = GetTextPhraseBundle( text );
    if ( phraseBundle == nullptr ) {
      // Add Phrase bundle to cache
      const auto kvp = _textPhraseCache.emplace( text, TextPhraseBundle() );
      phraseBundle = &kvp.first->second;
    }
    
    // Check if Audio data already for text/style pair
    state = phraseBundle->PrepareTextPhrase( style );
  }
  // Only create audio data if state is Preparing
  if ( state != SpeechState::Preparing ) {
    return state;
  }
  
  // Dispatch work onto another thread
  Util::Dispatch::Async( _dispatchQueue, [this, text, style]
  {
    auto audioData = CreateAudioData( text, style );
    {
      std::lock_guard<std::mutex> lock( _lock );
      TextPhraseBundle* phraseBundle = GetTextPhraseBundle( text );
      ASSERT_NAMED(phraseBundle != nullptr,
                   "TextToSpeechComponent.CreateSpeech.DispatchAysnc.PhraseBundleNotNull - \
                   Phrase Bundle should have been created before creating Audio Data");
      bool success = phraseBundle->SetStandardWaveDataContainer( audioData, style );
      // The Caller (this class) is responsible for data if not successful
      if ( !success ) {
        PRINT_NAMED_ERROR("TextToSpeechComponent.CreateSpeech.DispatchAysnc", "Must prepare phraseBundle before using");
        Util::SafeDelete( audioData );
      }
    }
  });
 
  return state;
} // CreateSpeech()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::SpeechState TextToSpeechComponent::GetSpeechState(const std::string& text, const SayTextStyle style)
{
  // Find phrase
  std::lock_guard<std::mutex> lock( _lock );
  TextPhraseBundle* phraseBundle = GetTextPhraseBundle( text );
  if ( phraseBundle == nullptr ) {
    return SpeechState::None;
  }
  
  return phraseBundle->GetSpeechState( style );
} // GetSpeechState()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TextToSpeechComponent::PrepareToSay(const std::string& text,
                                         SayTextStyle style,
                                         Audio::GameObjectType audioGameObject,
                                         float& out_duration_ms)
{
  std::lock_guard<std::mutex> lock( _lock );
  TextPhraseBundle* phraseBundle = GetTextPhraseBundle( text );
  auto waveData = phraseBundle->GetWaveData( style );
  if ( waveData == nullptr ) {
    return false;
  }
  
  using namespace Audio;
  ASSERT_NAMED(nullptr != _audioController, "TextToSpeechComponent.PrepareToSay.NullAudioController");
  AudioControllerPluginInterface* pluginInterface = _audioController->GetPluginInterface();
  ASSERT_NAMED(pluginInterface != nullptr, "TextToSpeechComponent.PrepareToSay.NullAudioControllerPluginInterface");
  
  // Clear previously loaded data
  if ( pluginInterface->WavePortalHasAudioDataInfo() ) {
    pluginInterface->ClearWavePortalAudioDataInfo();
  }
  // Set OUT value
  out_duration_ms = waveData->ApproximateDuration_ms();
  // Set audio data in plugin buffer
  pluginInterface->SetWavePortalAudioDataInfo( waveData->sampleRate,
                                               waveData->numberOfChannels,
                                               out_duration_ms,
                                               waveData->audioBuffer,
                                               static_cast<uint32_t>(waveData->bufferSize) );
  
  // Set audio RTPC for event
  PrepareSpeechAudioRtpc( style, audioGameObject, out_duration_ms );

  return true;
} // PrepareToSay()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::ClearLoadedSpeechData(const std::string& text, SayTextStyle style)
{
  std::lock_guard<std::mutex> lock( _lock );
  TextPhraseBundle* phraseBundle = GetTextPhraseBundle( text );
  phraseBundle->ClearTextPhrase( style );
} // ClearLoadedSpeechData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::ClearAllLoadedAudioData()
{
  std::lock_guard<std::mutex> lock( _lock );
  _textPhraseCache.clear();
} // ClearAllLoadedAudioData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Audio::GameEvent::GenericEvent TextToSpeechComponent::GetAudioEvent(SayTextStyle style)
{
  switch ( style ) {
    case SayTextStyle::Normal:
      // TODO: Create an event for non-name text to speech
      return Audio::GameEvent::GenericEvent::Play__Dev_Robot__External_Source;
      break;
      
    case SayTextStyle::Name_Normal:
      return Audio::GameEvent::GenericEvent::Play__Robot_Vo__External_Cozmo_Processing;
      break;
      
    case SayTextStyle::Name_FirstIntroduction:
      return Audio::GameEvent::GenericEvent::Play__Robot_Vo__External_Cozmo_Processing;
      break;
      
    case SayTextStyle::Count:
      PRINT_NAMED_ERROR("TextToSpeechComponent.GetAudioEvent", "Invalid SayTextStyle Count");
      break;
  }
  return Audio::GameEvent::GenericEvent::Play__Dev_Robot__External_Source;
} // GetAudioEvent()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Audio::StandardWaveDataContainer* TextToSpeechComponent::CreateAudioData(const std::string& text, const SayTextStyle style)
{
  using namespace Util::Time;
  unsigned long long int time = 0;
  if ( DEBUG_TEXTTOSPEECH_COMPONENT ) {
    
    time = UniversalTime::GetCurrentTimeInNanoseconds();
    PRINT_NAMED_DEBUG("TextToSpeechComponent.CreateAudioData",
                      "Start - text to wave: %s - style: %hhu", text.c_str(), style);
  }
  
  // TODO: Need to handle Style when generating Wave Data
  cst_wave* waveData = flite_text_to_wave( text.c_str(), _voice );
  
  if ( DEBUG_TEXTTOSPEECH_COMPONENT ) {
    PRINT_NAMED_DEBUG("TextToSpeechComponent.CreateAudioData",
                      "finish text to wave - time_ms: %f", UniversalTime::GetNanosecondsElapsedSince(time) * 1E-6f );
  }
  
  if ( waveData->num_samples == 0) {
    delete_wave( waveData );
    return nullptr;
  }
  
  // Create Standard Wave
  Audio::StandardWaveDataContainer* data = new Audio::StandardWaveDataContainer( waveData->sample_rate,
                                                                                 waveData->num_channels,
                                                                                 (size_t)waveData->num_samples );

  // Convert waveData format into StandardWaveDataContainer's format
  const float kOneOverSHRT_MAX = 1.0 / float(SHRT_MAX);
  for ( size_t sampleIdx = 0; sampleIdx < data->bufferSize; ++sampleIdx ) {
    data->audioBuffer[sampleIdx] = waveData->samples[sampleIdx] * kOneOverSHRT_MAX;
  }
  
  if ( DEBUG_TEXTTOSPEECH_COMPONENT ) {
    PRINT_NAMED_DEBUG("TextToSpeechComponent.CreateAudioData",
                      "Finish convert samples - time_ms: %f", UniversalTime::GetNanosecondsElapsedSince(time) * 1E-6f );
  }
  
  delete_wave( waveData );
  
  return data;
} // CreateAudioData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::PrepareSpeechAudioRtpc(const SayTextStyle style, Audio::GameObjectType audioGameObject, float duration_ms)
{
  // This is only used when saying names
  if ( style == SayTextStyle::Name_Normal || style == SayTextStyle::Name_FirstIntroduction ) {
    using namespace Audio;
    ASSERT_NAMED(nullptr != _audioController, "TextToSpeechComponent.PrepareSpeechAudioRtpc.NullAudioController");
    
    const AudioEngine::AudioParameterId parameterId = static_cast<const AudioEngine::AudioParameterId>(GameParameter::ParameterType::External_Name_Duration);
    const AudioEngine::AudioRTPCValue rtpcValue = Util::numeric_cast<const AudioEngine::AudioRTPCValue>( duration_ms * 0.001f );
    const AudioEngine::AudioGameObject gameObject = static_cast<const AudioEngine::AudioGameObject>(audioGameObject);
    _audioController->SetParameter(parameterId, rtpcValue, gameObject);
  }
} // PrepareSpeechAudioRtpc()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::TextPhraseBundle* TextToSpeechComponent::GetTextPhraseBundle(const std::string& text)
{
  const auto iter = _textPhraseCache.find( text );
  if ( iter != _textPhraseCache.end() ) {
    return &iter->second;
  }
  
  return nullptr;
} // GetTextPhraseBundle()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string TextToSpeechComponent::CreateHashForText(const std::string& text)
{
  // In case text contains sensitive data (e.g. player name), hash it so we don't save names
  // into logs if/when there's a message about this filename
  // TODO: Do we need something more secure?
  u32 hashedValue = 0;
  for ( auto& c : text ) {
    Util::AddHash( hashedValue, c, "TextToSpeechComponent.CreateHashForText.HashText" );
  }
  
  // Note that this text-to-hash mapping is only printed in debug mode!
  PRINT_NAMED_DEBUG("TextToSpeechComponent.CreateHashForText.TextToHash",
                    "'%s' hashed to %u", text.c_str(), hashedValue);
  
  return std::to_string( hashedValue );
} // CreateHashForText()


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TextPhraseBundle Class Implementation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
TextToSpeechComponent::SpeechState TextToSpeechComponent::TextPhraseBundle::GetSpeechState(const SayTextStyle style)
{
  PhraseCreationStyleBinding* phraseBinding = GetPhraseCreationStyleBinding( style );
  if ( phraseBinding == nullptr ) {
    return SpeechState::None;
  }
  return phraseBinding->state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Audio::StandardWaveDataContainer* TextToSpeechComponent::TextPhraseBundle::GetWaveData(const SayTextStyle style)
{
  PhraseCreationStyleBinding* phraseBinding = GetPhraseCreationStyleBinding( style );
  if ( phraseBinding == nullptr ) {
    return nullptr;
  }
  return phraseBinding->waveData;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::SpeechState TextToSpeechComponent::TextPhraseBundle::PrepareTextPhrase(const SayTextStyle style)
{
  TextAudioCreationStyle creationStyle = GetTextAudioCreationStyle( style );
  auto iter = _phraseStyleMap.find( creationStyle );
  if ( iter == _phraseStyleMap.end() ) {
    // Create entry for text creation style
    PhraseCreationStyleBinding binding;
    binding.state = SpeechState::Preparing;
    iter = _phraseStyleMap.emplace( creationStyle, binding ).first;
  }
  // Track Styles using Phrase Style Binding
  iter->second.registeredStyleSet.insert( style );
  
  return iter->second.state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TextToSpeechComponent::TextPhraseBundle::SetStandardWaveDataContainer(Audio::StandardWaveDataContainer* waveData , const SayTextStyle style)
{
  ASSERT_NAMED( waveData != nullptr, "WaveData can NOT == nullptr" );
  // Check if audio data already exist
  PhraseCreationStyleBinding* phraseBinding = GetPhraseCreationStyleBinding( style );
  if ( phraseBinding == nullptr ) {
    // Test audio style is NOT prepared
    return false;
  }
  const auto styleIter = phraseBinding->registeredStyleSet.find( style );
  if ( styleIter == phraseBinding->registeredStyleSet.end() ) {
    PRINT_NAMED_WARNING("TextPhraseBundle.SetStandardWaveDataContainer",
                        "Setting wave data for SayTextStyle [%hhu] that has not been registered. We are able to \
                        continue because another SayTextStyle has already created the necessary Wave Data", style);
  }
  
  // Add Wave data
  phraseBinding->state = SpeechState::Ready;
  phraseBinding->waveData = waveData;
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechComponent::TextPhraseBundle::ClearTextPhrase(const SayTextStyle style)
{
  // Find TextAudioCreationStyle PhraseCreationStyleBinding for SayTextStyle
  auto phraseStyleIter = _phraseStyleMap.find( GetTextAudioCreationStyle( style ) );
  if ( phraseStyleIter == _phraseStyleMap.end() ) {
    PRINT_NAMED_WARNING("TextPhraseBundle.ClearTextPhrase",
                        "Could NOT find TextAudioCreationStyle for SayTextStyle [%hhu]", style);
    return;
  }
  // Find registered SayTextStyle in PhraseCreationStyleBinding
  auto& registeredStyleSet = phraseStyleIter->second.registeredStyleSet;
  const auto iter = registeredStyleSet.find( style );
  if ( iter == registeredStyleSet.end() ) {
    PRINT_NAMED_WARNING("TextPhraseBundle.ClearTextPhrase", "Could NOT find registered SayTextStyle [%hhu]", style);
  }
  
  // Unregister SayTextStyle in PhraseCreationStyleBinding
  registeredStyleSet.erase(iter);
  // If no other styles are using Creation Style, delete entry
  if ( registeredStyleSet.empty() ) {
    _phraseStyleMap.erase( phraseStyleIter );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::TextPhraseBundle::TextAudioCreationStyle TextToSpeechComponent::TextPhraseBundle::GetTextAudioCreationStyle(const SayTextStyle style) const
{
  // TODO: need method to map from SayTextStyle -> TextAudioCreationStyle
  return TextAudioCreationStyle::Normal;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechComponent::TextPhraseBundle::PhraseCreationStyleBinding* TextToSpeechComponent::TextPhraseBundle::GetPhraseCreationStyleBinding(const SayTextStyle style)
{
  auto phraseStyleIter = _phraseStyleMap.find( GetTextAudioCreationStyle( style ) );
  if ( phraseStyleIter == _phraseStyleMap.end() ) {
    return nullptr;
  }
  return &phraseStyleIter->second;
}

} // end namespace Cozmo
} // end namespace Anki

